#include <defs.h>
#include <mram.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <perfcounter.h>
#include <string.h>
#include <mutex.h>
#include "common.h"
#include <alloc.h>
#include <built_ins.h>
#include <defs.h>
#include <seqread.h>

#include "../../common/include/common.h"
#include "strstr_dpu.h"

/* dpu marcos */
#define BLOCK_SIZE 1024 
#define MAX_BODY_ALLOCTE 4096
#define MIN_RECORDS_LENGTH 8
#define STRSTR strstr_org


/* global variables */
__host uint32_t input_length = 0;
__host uint32_t output_length = 0;
__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t key_cache[MAX_KEY_SIZE];
__host __mram_ptr uint8_t *DPU_BUFFER;
__host __mram_ptr uint8_t *RECORDS_BUFFER;
__mram_noinit uint8_t KEY[MAX_KEY_SIZE];
__host uint32_t input_offset[NR_TASKLETS];

MUTEX_INIT(write_mutex);


# if 1
static int find_next_set_bit(unsigned int res, int start) {
    if(start == 3) {
        return 4;
    }
    
    for (int i=start; i< 4; i++){
        if(res & (0x01<<((3-i)*8))){
            return i;
        }
    }
    return 4;
}
#endif

void shift_same(uint8_t* start, unsigned int *a){
    *a = 0;
    *a |= (start[0]<< 24);
    *a |= (start[0]<< 16);
    *a |= (start[0]<< 8);
    *a |= (start[0]);
}


uint32_t roundUp8(uint32_t a) {
    return a+8-a%8;
}


/*
 * debug function for displaying records
 */
void printRecord(uint8_t* record_start, uint32_t length) {
    for(int i =0; i< (int)length; i++){
        //printf("%c", record_start[i]);
        char c = record_start[i];
        putchar(c);
    }
    printf("\n");
    // printf("t %d\n", me());
    printf("\n");
}

/*
 * Parse search string from mram
 */
bool parseKey() {
    mram_read(&(KEY[0]), key_cache, MAX_KEY_SIZE);
    uint8_t* end = memchr(key_cache,'\n' ,MAX_KEY_SIZE);

    if(end != NULL) {
        *((char*)(end)) = '\0';
        return true;
    }
    else {
        printf("invalid key\n");
        return false;
    }
}


unsigned int READ_4_BYTE(struct in_buffer_context *_i) {
   unsigned int ret = 0;
   int i =0;

   do {
        uint8_t a = *_i->ptr;
        ret |= a << 8 *(3-i);
        _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t), &_i->sr);

        i++;
   } while(i<4);

    return ret;
}


void READ_X_BYTE(unsigned int *a, struct in_buffer_context *_i, int len) {
    int i =4-len;
    *a = *a << len*8;
    // *a = *a << len*8; 
   do {

        *a = *a | (*_i->ptr) << (8 *(3-i));
        _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t), &_i->sr);
        // printf("READ_X_BYTE a %X i %d len %d\n", *a, i, len);
        i++;
   } while(i<4);
}


bool STRSTR_4_BYTE(unsigned int a, int* next){
    int j=0;
    unsigned int res = 0;
    unsigned int b = 0;

    for(j=0; j< 4; j++) {
        shift_same(key_cache+j, &b);
        __builtin_cmpb4_rrr(res, a, b);
        // jth byte matches
        if(res & (0x01<<((3-j)*8))){
            continue;
        }
        else {
            // 00010001 => 1
            *next = find_next_set_bit(res, j);
            break;
        }
    }

    if (j==4) {
        return true;
    }    

    return false; 
}

/* check if the record contains \n */
bool CHECK_4_BYTE(unsigned int a, int *kth_byte) {

    unsigned int b = 0x0A0A0A0A;
    unsigned int res = 0;
    
    // shift_same('\n', &b);
    __builtin_cmpb4_rrr(res, a, b);
    // printf("res %x a %x \n", res, a);
    // there's a \n exist   
    if(res != 0) {

        *kth_byte = 0;
        for(int k=0; k<4; k++) {
        // locate the \n byte
            if(((res>>((3-k)*8)) & 0x01) != 0) {
                *kth_byte = k;
            }
        }
    
        return true;
    }

    return false;
}

void mv_cache_to_mram(uint8_t *cache, uint32_t length) {
    uint32_t temp = output_length;
    output_length +=  roundUp8(length);
    mram_write(cache, RECORDS_BUFFER+temp, roundUp8(length));
    // printRecord(cache, length);
}




void write_to_mram(struct record_descrip * rec, struct in_buffer_context *_i) {
    // seek back to memory start
    uint8_t * cache = DPU_CACHES[me()];

    _i->ptr =  seqread_seek(rec->record_start, &_i->sr);
    uint32_t i = 0;
    uint32_t c_i = 1;
    uint8_t prev_char = *(_i->ptr); 
    cache[0] = '\n';
    // get a lock here

    do {

        if( c_i < BLOCK_SIZE) {
            // prev_char = cache[c_i]; 
            cache[c_i] = *(_i->ptr);

            _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t), &_i->sr);

            // if(i > 2873 && i< 2890) {
            //     printf("cache %x prev %x\n", cache[c_i], prev_char);
            // }
            if(cache[c_i] == 0x0A && prev_char == 0x7D) {
                mv_cache_to_mram(cache, c_i);
                rec->length += c_i;
                // printf("------ record finishes %d c_i %d %c\n", rec->length, c_i, *(_i->ptr));
                return;
            }

        }
        else {
            mv_cache_to_mram(cache, c_i);
            rec->length += c_i;
            c_i = 0;
            continue;
        }
        prev_char = cache[c_i];
        i++;
        c_i++;
        // printf("------ record length %d\n", rec->length);
    } while (i < _i->length);
}


bool dpu_strstr(struct in_buffer_context *input) {
    // int j = 0; 
    // unsigned int res = 0;
    // unsigned int b = 0x0A0A0A0A;
    int next = 0;
    struct record_descrip rec;
    rec.record_start = input->mram_org;
    rec.state = 1;
    rec.org = input->curr;
    rec.length =0;
    // int dbg_cnt = 0;
    // uint8_t pre_char = 0x0;
    
    unsigned int a = READ_4_BYTE(input);
    // printf("init a %X\n", a);

    do {
        if (STRSTR_4_BYTE(a, &next)) {
            rec.str_found = true;
            // stop reading, move to copy
            printf("strstr found\n");
            // incremement bench of counters
            mutex_lock(write_mutex);
            write_to_mram(&rec, input);
            mutex_unlock(write_mutex);
            // break;
            // reset
            rec.record_start += rec.length;

            rec.str_found = false;
            next = 4;
            input->curr = rec.org + rec.length;
            rec.org = input->curr;
            printf("strstr record length %d curr %d org %d\n", rec.length, input->curr, rec.org);

            rec.length =0;

        }
        else {
            int kth_byte = 0;
            if(CHECK_4_BYTE(a, &kth_byte)) {
                printf("goes to here\n");
                // finish reading a record
                
                rec.length = input->curr - rec.org -(4-kth_byte-1);
                printf("record length %d curr %d org %d kth_byte%d \n", rec.length, input->curr, rec.org, kth_byte);
                // reset
                rec.record_start += (rec.length);
                rec.str_found = 0;

                rec.org += rec.length;
                rec.length =0;
                next = (kth_byte+1);  
            }   
        }

        // printf("init a %X next %d \n", a, next);
        // pre_char = (a>>((4-next)*8)) & (0xFF);

        switch (next) {
            case 1:
            case 2:
            case 3:

                READ_X_BYTE(&a, input, next);
                input->curr += next;
                break;
            case 4: 
                a = READ_4_BYTE(input);
                input->curr +=4;
                break;
        }
        // printf("a %x  prechar %x next %d\n", a, pre_char, next);
      
        //  += 4- next;

        // if(dbg_cnt == 72000) {

        //     break;
        // }
        // dbg_cnt++;

    } while(input->curr < input->length|| input->curr+4 < input->length);

    return false;
}


int main()
{
    uint8_t idx = me();
	printf("DPU starting, tasklet %d input_offset %d\n", idx, input_offset[idx]);

    if(idx == 0) {
        if(!parseKey()) {
            printf("dpu parse search string failed return\n");
            return 0;
        }
    }

	// Check that this tasklet has work to run 
	if ((idx != 0) && (input_offset[idx] == 0)) {
		printf("Tasklet %d has nothing to run\n", idx);
		return 0;
	}   
    struct in_buffer_context input;
    uint32_t input_start = input_offset[idx] - input_offset[0];

    input.cache = seqread_alloc();
    input.ptr = seqread_init(input.cache, DPU_BUFFER + input_start, &input.sr);
    input.mram_org = DPU_BUFFER + input_start;
    input.curr = 0;
	input.length = 0;

	// Calculate the actual length this tasklet parses
	if (idx < (NR_TASKLETS - 1)) {
		int32_t input_end = input_offset[idx + 1] - input_offset[0];

		// If the end position is negative, then the next task has no work
		// to run. Use the remainder of the input length to calculate this
		// task's length.
		// if ((input_end <= 0)) {
		// 	input.length = input_length - input_start;
		// }
		// else {
		input.length = input_end - input_start;
		// }
	}
	else {
		input.length = input_length - input_start; 
        printf("input_start: %u length: %u\n", input_start, input_length);
	}
#if 1
    // dbg_printf("key cache %c%c%c%c \n", key_cache[0], key_cache[1], key_cache[2], key_cache[3]);
    
    
    perfcounter_config(COUNT_CYCLES, true);
    if (input.length != 0) {
		// Do the uncompress
		if (dpu_strstr(&input))
		{
			printf("Tasklet %d: found searched pattern %lu\n", idx, perfcounter_get());
            return -1;
		}
	}
#endif
}



#if 0
bool dpu_strstr(struct in_buffer_context *input) {
    int j = 0; 
    unsigned int res = 0;
    unsigned int b = 0;
    int next = 0;

#if 1
    do {
        #if 1
        unsigned int a = READ_4_BYTE(input);

        // input->ptr = (uint8_t*)p_str;
        for(j=0; j< 4; j++) {
                shift_same(key_cache+j, &b);
                __builtin_cmpb4_rrr(res, a, b);
                // jth byte matches
                if(res & (0x01<<((3-j)*8))){
                    continue;
                }
                else {
                    // 00010001 => 1
                    next = find_next_set_bit(res, j);
                    break;
                }
            }
        #endif
        if (j==4) {
            return true;
        }

        if(next != 4) {
            input->ptr -= next;  
        }
  
        // if(me() == 0){
        //     dbg_printf("tasklet %d: sequential reader reads %x count %d, next %d \n", me(), a, 0, next);
        // }
        
        input->curr += 4- next;

        // if(dbg_cnt == 3) {
        //     break;
        // }
        // dbg_cnt++;
    } while(input->curr < input->length|| input->curr+4 < input->length);
#endif
    return false;
}
#endif


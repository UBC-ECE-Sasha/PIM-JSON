#include <defs.h>
#include <mram.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <perfcounter.h>
#include <string.h>
#include <mutex.h>
#include "../../dpu_common.h"
#include <alloc.h>
#include <built_ins.h>
#include <defs.h>
#include <seqread.h>

#include "strstr_dpu.h"

/* dpu macros */
#define BLOCK_SIZE 1024 
#define MAX_BODY_ALLOCTE 4096
#define MIN_RECORDS_LENGTH 8
#define STRSTR strstr_org
#define MAX_KEY_ARY_LENGTH 8


/* global variables */
__host long input_length = 0;
__host uint32_t output_length = 0;
__host int adjust_offset = 0;
uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__host unsigned int  key_cache[MAX_KEY_ARY_LENGTH];
__host __mram_ptr uint8_t *DPU_BUFFER;
__host __mram_ptr uint8_t *RECORDS_BUFFER;
__host long input_offset[NR_TASKLETS];

MUTEX_INIT(write_mutex);

/**
 * 
 * Utility functions 
 *  
 **/
static int find_next_set_bit(unsigned int res, int start) {
    if(start == 3) {
        return 4;
    }
    
    for (int i=start; i< 4; i++){
        if(res & (0x01<<((3-i)<<3))){
            return i;
        }
    }
    return 4;
}


void shift_same(uint8_t start, unsigned int *a){
    *a = 0;
    *a |= (start<< 24);
    *a |= (start<< 16);
    *a |= (start<< 8);
    *a |= (start);
}


uint32_t roundUp8(uint32_t a) {
    return a+8-a%8;
}


/*
 * debug function for displaying records
 */
void printRecord(uint8_t* record_start, uint32_t length) {
    for(int i =0; i< (int)length; i++){
        char c = record_start[i];
        putchar(c);
    }
    printf("\n");
    printf("\n");
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
    *a = *a << (len<<3);
   do {

        *a = *a | (*_i->ptr) << (8 *(3-i));
        _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t), &_i->sr);
        i++;
   } while(i<4);
}


bool STRSTR_4_BYTE(unsigned int a, int* next){
    int j=0;
    unsigned int res = 0;
    unsigned int b = 0;

    for(j=0; j< 4; j++) {
        shift_same(key_cache[0]>>((3-j)<<3), &b);
        __builtin_cmpb4_rrr(res, a, b);
        //TODO op - needs more test
        if(j==0) {
            *next = find_next_set_bit(res, j+1);
        }

        // jth byte matches
        if(res & (0x01<<((3-j)<<3))){
            continue;
        }
        else {
            // *next = find_next_set_bit(res, j);
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

    __builtin_cmpb4_rrr(res, a, b);
    // there's a \n exist   
    if(res != 0) {

        *kth_byte = 0;
        for(int k=0; k<4; k++) {
        // locate the \n byte
            if(((res>>((3-k)<<3)) & 0x01) != 0) {
                *kth_byte = k;
            }
        }
    
        return true;
    }

    return false;
}

void mv_cache_to_mram(uint8_t *cache, uint32_t length, bool is_end) {
    uint32_t temp = output_length;
    if(is_end) {
    	output_length += roundUp8(length);
    }
    else {	    
    	output_length +=  length;
    }    
    mram_write(cache, RECORDS_BUFFER+temp, roundUp8(length));
}


void write_to_mram(struct record_descrip * rec, struct in_buffer_context *_i) {
    // seek back to memory start
    uint8_t * cache = DPU_CACHES[me()];

    _i->ptr =  seqread_seek(rec->record_start, &_i->sr);
    uint32_t i = 0;
    uint32_t c_i = 1;
    uint8_t prev_char = *(_i->ptr); 
    cache[0] = '\n';

    do {

        if( c_i < BLOCK_SIZE) {
            cache[c_i] = *(_i->ptr);
            _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t), &_i->sr);
        
            if(cache[c_i] == 0x0A && prev_char == 0x7D) {
                mv_cache_to_mram(cache, c_i, true);
                rec->length += c_i;
                return;
            }

        }
        else {
            mv_cache_to_mram(cache, c_i, false);
            rec->length += c_i;
            c_i = 0;
            continue;
        }
        prev_char = cache[c_i];
        i++;
        c_i++;
    } while (i < _i->length);
    // output_length = roundUp8(output_length+64);
}


/************************************************************************************************** 
*
*
* main code starts here
*
*
****************************************************************************************************/
bool dpu_strstr(struct in_buffer_context *input) {
    int next = 0;
    struct record_descrip rec;
    rec.record_start = input->mram_org;
    rec.state = 1;
    rec.org = input->curr;
    rec.length =0;
    
    unsigned int a = READ_4_BYTE(input);

    do {
        if (STRSTR_4_BYTE(a, &next)) {
            rec.str_found = true;
            // stop reading, move to copy
            dbg_printf("strstr found\n");

            mutex_lock(write_mutex);
            write_to_mram(&rec, input);
            mutex_unlock(write_mutex);

            rec.record_start += rec.length;
            rec.str_found = false;
            next = 4;
            input->curr = rec.org + rec.length;
            rec.org = input->curr;
            dbg_printf("strstr record length %d curr %d org %d\n", rec.length, input->curr, rec.org);
            rec.length =0;
        }
        else {
            int kth_byte = 0;
            if(CHECK_4_BYTE(a, &kth_byte)) {
                // finish reading a record                
                rec.length = input->curr - rec.org -(4-kth_byte-1);
                dbg_printf("record length %d curr %d org %d kth_byte%d \n", rec.length, input->curr, rec.org, kth_byte);
                // reset
                rec.record_start += (rec.length);
                rec.str_found = 0;

                rec.org += rec.length;
                rec.length =0;
                next = (kth_byte+1);  
            }   
        }

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
    } while(input->curr < input->length|| input->curr+4 < input->length);

    return false;
}


int main()
{
    uint8_t idx = me();
	dbg_printf("DPU starting, tasklet %d input_offset %d\n", idx, input_offset[idx]);

	// Check that this tasklet has work to run 
	if ((idx != 0) && (input_offset[idx] == 0)) {
		printf("Tasklet %d has nothing to run\n", idx);
		return 0;
	}   
    struct in_buffer_context input;
    long input_start = input_offset[idx] - input_offset[0];

    input.cache = seqread_alloc();
    input.mram_org = DPU_BUFFER + input_start + adjust_offset;
    input.ptr = seqread_init(input.cache, input.mram_org, &input.sr);
    input.curr = 0;
	input.length = 0;

	// Calculate the actual length this tasklet parses
	if (idx < (NR_TASKLETS - 1)) {
		long input_end = input_offset[idx + 1] - input_offset[0];
		input.length = input_end - input_start;
	}
	else {
		input.length = input_length - input_start - adjust_offset; 
        dbg_printf("input_start: %u length: %u\n", input_start, input_length);
	}
#if 1    
    perfcounter_config(COUNT_CYCLES, true);
    if (input.length != 0) {
		// Do the uncompress
		if (dpu_strstr(&input))
		{
			dbg_printf("Tasklet %d: found searched pattern %lu\n", idx, perfcounter_get());
            return -1;
		}
	}
#endif
}
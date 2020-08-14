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
#define SEQREAD_CACHE_SIZE S_BUFFER_LENGTH
#include <seqread.h>

#include "strstr_dpu.h"

/* dpu macros */
#define MIN_RECORDS_LENGTH 8
#define MAX_KEY_ARY_LENGTH 8
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define MASK_BIT(var,pos) ((var) |= (1<<(pos)))
#define SEQ_4_OP2


/* global variables */
__host uint32_t input_length = 0;
__host uint32_t output_length = 0;
__host uint32_t adjust_offset = 0;
__host uint32_t query_count = 0;
__host unsigned int  key_cache[MAX_KEY_ARY_LENGTH];
unsigned int key_char[MAX_KEY_ARY_LENGTH];
uint8_t __mram_noinit DPU_BUFFER[MEGABYTE(58)];
__mram_noinit struct json_candidate candidates[MAX_NUM_RETURNS] = {0};
__host uint32_t input_offset[NR_TASKLETS];
__host volatile uint32_t offset_count = 0;

MUTEX_INIT(write_mutex);

/**
 * 
 * Utility functions 
 *  
 **/
static int find_next_set_bit(unsigned int res) {
#if HOST_DEBUG
    for (int i=1; i< 4; i++){
        if(res & (0x01<<((3-i)<<3))){
            return i;
        }
    }
    return 4;
#endif
    if (res & 65536) return 1;
    if (res & 256) return 2;
    if (res & 1) return 3;
    return 4;
}


static void shift_same(uint8_t start, unsigned int *a){
    *a = 0;
    *a |= (start<< 24);
    *a |= (start<< 16);
    *a |= (start<< 8);
    *a |= (start);
}

#if DEBUG
static uint32_t roundUp8(uint32_t a) {
    return a+8-a%8;
}


/*
 * debug function for displaying records
 */
static void printRecord(uint8_t* record_start, uint32_t length) {
    for(int i =0; i< (int)length; i++){
        char c = (char)record_start[i];
        putchar(c);
    }
    printf("\n");
    printf("\n");
}
#endif 
#ifdef SEQ_4_NOP
static unsigned int READ_4_BYTE(struct in_buffer_context *_i) {
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
#endif


#ifdef SEQ_4_OP2
void seqread_get_x(struct in_buffer_context *_i){
    if(_i->seq_cnt+(S_BUFFER_LENGTH-1) > _i->length) {
        // read left bytes
        
        _i->ptr = seqread_get(_i->ptr, _i->length-_i->seq_cnt, &_i->sr);
        _i->seq_cnt = _i->length;
    }
    else {
        _i->ptr = seqread_get(_i->ptr, S_BUFFER_LENGTH-1, &_i->sr);
        _i->seq_cnt +=(S_BUFFER_LENGTH-1);
   }
    _i->seqread_indx =0;
}
#endif


#ifdef SEQ_4_OP1
static unsigned int READ_4_BYTE_4(struct in_buffer_context *_i) {
    

    unsigned int ret = _i->ptr[0] << 24 |
                  (_i->ptr[1] << 16) |
                  (_i->ptr[2] << 8) | 
                  (_i->ptr[3]);
    _i->ptr = seqread_get(_i->ptr, sizeof(uint32_t), &_i->sr);
    return ret;
}
#endif


#ifdef SEQ_4_OP1
static void READ_X_BYTE_4(unsigned int *a, struct in_buffer_context *_i, int len) {

    int i = 4-len;
    int j = 0;
    *a = *a << (len<<3);
    do {
        *a = *a | (_i->ptr[j]) << (8 *(3-i));
        i++;
        j++;
    } while(i<4);
    _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t)*(len), &_i->sr);
}
#endif

#ifdef SEQ_4_NOP
static void READ_X_BYTE(unsigned int *a, struct in_buffer_context *_i, int len) {
    int i =4-len;
    *a = *a << (len<<3);
   do {

        *a = *a | (*_i->ptr) << (8 *(3-i));
        _i->ptr = seqread_get(_i->ptr, sizeof(uint8_t), &_i->sr);
        i++;
   } while(i<4);
}
#endif

#ifdef SEQ_4_OP2
static unsigned int READ_4_BYTE_4X(struct in_buffer_context *_i) {
    unsigned int ret = 0;

    if(_i->seqread_indx +4 > S_BUFFER_LENGTH -1) {
        // need to copy in bytes I need then reload cache
        uint32_t temp = (_i->seqread_indx +4) - (S_BUFFER_LENGTH -1); // bytes to read after cache reload
        uint32_t load_pre = 4 - temp; //bytes to read afterwards
        uint32_t i =0;
        if(load_pre != 0) {
            for (i=0; i< load_pre; i++) {
                ret |= _i->ptr[_i->seqread_indx + i] << ((3-i)<<3);
            }
        }
        seqread_get_x(_i); // cache reload
        for (uint32_t k=0; k<temp; k++) {
            ret |= _i->ptr[_i->seqread_indx + k] << ((3-i)<<3);
            i++;
        }
        _i->seqread_indx += temp;
    }
    else {
        // do the normal thing
        // increment counter
        ret = _i->ptr[_i->seqread_indx + 0] << 24 |
                    (_i->ptr[_i->seqread_indx + 1] << 16) |
                    (_i->ptr[_i->seqread_indx + 2] << 8) | 
                    (_i->ptr[_i->seqread_indx + 3]);
        _i->seqread_indx += 4;
    }   
    return ret;
}

static void READ_X_BYTE_4X(unsigned int *a, struct in_buffer_context *_i, int len) {
    int i = 4-len;
    int j = 0;
    *a = *a << (len<<3);

    if(_i->seqread_indx +len > S_BUFFER_LENGTH -1) {
        uint32_t temp = (_i->seqread_indx +len) - (S_BUFFER_LENGTH -1);
        uint32_t load_pre = len - temp;
        uint32_t k =0;
        if(load_pre != 0) {
            for (k=0; k< load_pre; k++) {
                *a |= _i->ptr[_i->seqread_indx + k] << ((3-i)<<3);
                i++;
            }            
        }
        seqread_get_x(_i);
        for (k=0; k< temp; k++) {
            *a |= _i->ptr[_i->seqread_indx + k] << ((3-i)<<3);
            i++;
        }        
        _i->seqread_indx += temp;
    }
    else {
        do {
            *a = *a | (_i->ptr[_i->seqread_indx+j]) << (8 *(3-i));
            i++;
            j++;
        } while(i<4);
        _i->seqread_indx += len;
    }

}
#endif


static bool STRSTR_4_BYTE_OP(unsigned int a, int* next, struct record_descrip* rec){
    unsigned int res = 0;
    unsigned int b = 0;
    int min_next = 5;

    for (uint32_t i=0; i< query_count; i++){
        // check if i bit masked? 
        if(CHECK_BIT(rec->str_mask, i)){
            continue;
        }
        else{
            // normal process
            // if true mask the bit correspand ot the key_cache
            // shift_same(key_cache[i]>>24, &b);
            b = key_char[i];
            __builtin_cmpb4_rrr(res, a, b);
            *next = find_next_set_bit(res);
            if(*next < min_next) {
                min_next = *next;
            }
                      
            if(res & 0x01000000) {
                // compare the following 4 bytes
                res = 0x0;
                __builtin_cmpb4_rrr(res, a, key_cache[i]);
                if(res == 0x01010101) {
                    *next = 4;
                    MASK_BIT(rec->str_mask, i);
                    return true;
                }
            }
        }
    }

    *next = min_next;
    return false;
}


/* check if the record contains \n */
static bool CHECK_4_BYTE(unsigned int a, int *kth_byte) {

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

/************************************************************************************************** 
*
*
* main code starts here
*
*
****************************************************************************************************/
static void CHECK_RECORD_END(unsigned int a, struct in_buffer_context *input, struct record_descrip* rec, int * next, uint32_t query_passed_count, uint8_t tasklet_id) {
    int kth_byte = 0;
    
    if(CHECK_4_BYTE(a, &kth_byte)) {
        // finish reading a record                
        rec->length = input->curr - rec->org + kth_byte+1;//-(4-kth_byte-1);
        if(query_passed_count >= query_count) {
            mutex_lock(write_mutex);
            candidates[offset_count].length = rec->length;
            candidates[offset_count].offset = rec->record_start - input->mram_org + input_offset[tasklet_id];
            offset_count++;
            mutex_unlock(write_mutex);  
            // RECORDS_OFFSETS[offset_count++] = rec.record_start - input->mram_org + input_offset[tasklet_id];
            // RECORDS_LENS[offset_count] = rec->length;   
           dbg_printf("tasklet %d records found %u count %u\n", me(),rec->record_start - input->mram_org + input_offset[tasklet_id], offset_count);
        }

        // reset
        rec->record_start += (rec->length);
        rec->org += rec->length;
        rec->length = 0;
        rec->str_mask = 0;
        *next = (kth_byte+1);

    }
}


static void dpu_strstr(struct in_buffer_context *input) {
    int next = 0;
    struct record_descrip rec;
    rec.record_start = input->mram_org;
    rec.state = 1;
    rec.org = input->curr;
    rec.length =0;
    rec.str_mask = 0;
    uint8_t tasklet_id = me();

    unsigned int a = READ_4_BYTE_4X(input);
    dbg_printf("tasklet %d reads %x\n", tasklet_id, a);

    uint32_t query_passed_count = 0;


    do { 
        // check how many queries have passed

        __builtin_cao_rr(query_passed_count, rec.str_mask);
        // check if queries exist in the current 4 bytes
        if ((query_passed_count < query_count) && STRSTR_4_BYTE_OP(a, &next, &rec)) {
             query_passed_count++;
            // update offset here

        }
        else {
            CHECK_RECORD_END(a, input, &rec, &next, query_passed_count, tasklet_id);
        }

        switch (next) {
            case 1:
            case 2:
            case 3:
                READ_X_BYTE_4X(&a, input, next);
                input->curr += next;
                break;
            case 4:
            default: 
                a = READ_4_BYTE_4X(input);
                input->curr +=4;
                break;
        }
    // if(input->curr > 280) {
    //     break;
    // }
    } while(input->curr < input->length|| input->curr+4 < input->length);
    dbg_printf("strstr tasklet %d length %u curr%u\n", me(), input->length, input->curr);

    // stop for seq_cnt + read_size > input->length
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
    uint32_t input_start = input_offset[idx];

    input.cache = seqread_alloc();
    input.mram_org = DPU_BUFFER + input_start;
    input.ptr = seqread_init(input.cache, input.mram_org, &input.sr);
    input.curr = 0;
	input.length = 0;
    input.seqread_indx = 0;
    input.seq_cnt = 0; // (S_BUFFER_LENGTH-1);

    for (uint32_t i = 0; i < query_count; i++) {
        shift_same(key_cache[i]>>24, &key_char[i]);
    }

	// Calculate the actual length this tasklet parses
	if (idx < (NR_TASKLETS - 1)) {
		input.length = input_offset[idx + 1] - input_offset[0]- input_start;
	}
	else {
		input.length = input_length - input_start; 
        dbg_printf("input_start: %u length: %u\n", input_start, input_length);
	}
#if 1    
    perfcounter_config(COUNT_INSTRUCTIONS, true);
    if (input.length != 0) {
		dpu_strstr(&input);
        dbg_printf("Tasklet %d: found searched pattern %lu %d bytes\n", idx, perfcounter_get(), input_length);
	}
#endif
    return 0;
}

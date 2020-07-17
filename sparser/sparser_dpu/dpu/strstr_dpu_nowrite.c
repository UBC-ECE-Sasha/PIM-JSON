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
#define MIN_RECORDS_LENGTH 8
#define MAX_KEY_ARY_LENGTH 8
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define MASK_BIT(var,pos) ((var) |= (1<<(pos)))


/* global variables */
__host uint32_t input_length = 0;
__host uint32_t output_length = 0;
__host uint32_t adjust_offset = 0;
__host uint32_t query_count = 0;
__host unsigned int  key_cache[MAX_KEY_ARY_LENGTH];
uint8_t __mram_noinit DPU_BUFFER[MEGABYTE(36)];
__mram_noinit uint32_t RECORDS_OFFSETS[MAX_NUM_RETURNS] = {0};
__host uint32_t input_offset[NR_TASKLETS];
__host uint32_t offset_count = 0;

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
        char c = (char)record_start[i];
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


bool STRSTR_4_BYTE_OP(unsigned int a, int* next, struct record_descrip* rec){
    unsigned int res = 0;
    unsigned int b = 0;

    for (uint32_t i=0; i< query_count; i++){
        // check if i bit masked? 
        if(CHECK_BIT(rec->str_mask, i)){
            continue;
        }
        else{
            // normal process
            // if true mask the bit correspand ot the key_cache
            shift_same(key_cache[i]>>24, &b);
            __builtin_cmpb4_rrr(res, a, b);
            *next = find_next_set_bit(res, 1);

            if(res & (0x01<<24)) {
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

/************************************************************************************************** 
*
*
* main code starts here
*
*
****************************************************************************************************/
void CHECK_RECORD_END(unsigned int a, struct in_buffer_context *input, struct record_descrip* rec, int * next) {
    int kth_byte = 0;
    
    if(CHECK_4_BYTE(a, &kth_byte)) {
        // finish reading a record                
        rec->length = input->curr - rec->org + kth_byte+1;//-(4-kth_byte-1);
        dbg_printf("record length %d curr %d org %d kth_byte%d \n", rec->length, input->curr, rec->org, kth_byte);

        // reset
        rec->record_start += (rec->length);
        rec->org += rec->length;
        rec->length = 0;
        rec->str_count = 0;
        rec->str_mask = 0;
        *next = (kth_byte+1);
    }

}


bool dpu_strstr(struct in_buffer_context *input) {
    int next = 0;
    struct record_descrip rec;
    rec.record_start = input->mram_org;
    rec.state = 1;
    rec.org = input->curr;
    rec.length =0;
    rec.str_count = 0;
    rec.str_mask = 0;
    uint8_t tasklet_id = me();

    unsigned int a = READ_4_BYTE(input);       
        
    do {    
        if ((rec.str_count < query_count) && STRSTR_4_BYTE_OP(a, &next, &rec)) {
            rec.str_count++;

            // update offset here
            if(rec.str_count == query_count) {
                mutex_lock(write_mutex);
                RECORDS_OFFSETS[offset_count++] = rec.record_start - input->mram_org + input_offset[tasklet_id];
                mutex_unlock(write_mutex);
                dbg_printf("records found %u count %u\n", rec.record_start - input->mram_org, offset_count);
            }
        }
        else {
            CHECK_RECORD_END(a, input, &rec, &next);
        }

        switch (next) {
            case 1:
            case 2:
            case 3:

                READ_X_BYTE(&a, input, next);
                input->curr += next;
                break;
            case 4:
            default: 
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
    uint32_t input_start = input_offset[idx];

    input.cache = seqread_alloc();
    input.mram_org = DPU_BUFFER + input_start;
    input.ptr = seqread_init(input.cache, input.mram_org, &input.sr);
    input.curr = 0;
	input.length = 0;

	// Calculate the actual length this tasklet parses
	if (idx < (NR_TASKLETS - 1)) {
		input.length = input_offset[idx + 1] - input_offset[0]- input_start;
	}
	else {
		input.length = input_length - input_start; 
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
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

/* dpu marcos */
#define BLOCK_SIZE 2048 
#define MAX_BODY_ALLOCTE 4096
#define MIN_RECORDS_LENGTH 8
#define STRSTR strstr_org


/* global variables */
__host uint32_t input_length = 0;
__host uint32_t output_length = 0;
__dma_aligned uint8_t key_cache[MAX_KEY_SIZE];
__host __mram_ptr uint8_t *DPU_BUFFER;
__host __mram_ptr uint8_t *RECORDS_BUFFER;
__mram_noinit uint8_t KEY[MAX_KEY_SIZE];
__host uint32_t input_offset[NR_TASKLETS];

typedef struct in_buffer_context
{
	uint8_t *ptr;
	seqreader_buffer_t cache;
	seqreader_t sr;
    __mram_ptr uint8_t *mram_org;
    uint32_t curr;
	uint32_t length;
} in_buffer_context;

// Return values
typedef enum {
	STRSTR_JSON_ok = 0, // Success code
	STRSTR_JSON_INVALID_INPUT,		// Input file has an invalid format
} strstr_status;

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
		if ((input_end <= 0)) {
			input.length = input_length - input_start;
		}
		else {
			input.length = input_end - input_start;
		}
	}
	else {
		input.length = input_length - input_start; 
        // dbg_printf("input_start: %u length: %u\n", input_start, input_length);
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

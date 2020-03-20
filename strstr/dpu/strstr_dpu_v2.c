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

#define BLOCK_SIZE 2048 

#define RETURN_RECORDS_SIZE 4<<15
#define MAX_BODY_ALLOCTE 4096
#define MIN_RECORDS_LENGTH 8

__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t key_cache[MAX_KEY_SIZE];
__mram_noinit uint8_t DPU_BUFFER[BUFFER_SIZE];
__mram_noinit uint8_t RECORDS_BUFFER[RETURN_RECORDS_SIZE];
__mram_noinit uint8_t KEY[MAX_KEY_SIZE];
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];


/*
 * Parse search string from mram
 */ 
bool parseKey() {
    //MRAM_READ((mram_addr_t)&(KEY[0]), key_cache, MAX_KEY_SIZE);
    mram_read(&(KEY[0]), key_cache, MAX_KEY_SIZE);
    uint8_t* end = memchr(key_cache,'\n' ,MAX_KEY_SIZE);

    if(end != NULL) {
        *((char*)(end)) = '\0';
        printf("valid key\n");
        return true;
    }
    else {
        printf("invalid key\n");
        return false;
    }
  
}


void printRecord(uint8_t* record_start, uint32_t length) {
    for(uint32_t i =0; i< length; i++){
        printf("%c", record_start[i]);
    }

    printf("\n");
    printf("\n");
}

bool parseJson(uint32_t start, uint32_t offset, uint8_t* cache) {
    int tasklet_id = me();
    uint8_t __mram_ptr * star = &(DPU_BUFFER[start]);
    uint8_t __mram_ptr * max_addr =  &DPU_BUFFER[BUFFER_SIZE-1];

    mram_read(star, cache, BLOCK_SIZE);
    uint32_t read_adjust_offset = BLOCK_SIZE;

    if(tasklet_id !=0) {
        int copy_size = BLOCK_SIZE;
        mram_read(star, cache, BLOCK_SIZE);
        // in case we read over
        if(&DPU_BUFFER[start+BLOCK_SIZE] >= max_addr) {
            copy_size = BUFFER_SIZE - start;
        }
        
        uint8_t* valid_start  = (uint8_t *)memchr(cache, '\n', copy_size);

        if(valid_start == NULL) {
            printf("thread %d could not find newline char returning early\n", tasklet_id);
            return false;
        }
        else {
            star = &DPU_BUFFER[valid_start - cache +1];
        }
    }

   uint8_t __mram_ptr * end;
   uint8_t* record_end;
   uint32_t record_count =0;
   if( tasklet_id == (NR_TASKLETS-1) ){
     end = max_addr;
   } else {
        uint8_t __mram_ptr * end_pos = star+ offset-1;  /* might get lucky and end up perfectly aligned, so check that char */

        do {
            mram_read(star, cache, BLOCK_SIZE);
            record_end =  memchr(cache, '\n', BLOCK_SIZE);
            uint8_t* block_end = cache+BLOCK_SIZE-1;
            if(!record_end) {
                printf("thread %d could not find newline char returning early stage 2\n", tasklet_id);
                return false;
            } 
            else {
                uint32_t record_length = record_end - cache;
                if(record_length < MIN_RECORDS_LENGTH){
                     printf("thread %d address shift\n", tasklet_id);
                }
                else {
                    printRecord(cache, record_length);
                    printf("thread %d found record %d\n", tasklet_id, record_length);
                    record_count++;
                }
                // printRecord(cache, record_length);

                uint32_t total = record_length;
                // search within cache for anothe records                
                do {
                    uint8_t * record_start = (uint8_t*)(record_end+1);
                    if(record_start >block_end) {
                        read_adjust_offset = BLOCK_SIZE;
                        break;
                    } 
                    record_end = memchr(record_start, '\n', BLOCK_SIZE-total);
                    // if no records
                    if(!record_end) {
                        read_adjust_offset = record_start-cache;
                        uint32_t org = read_adjust_offset;
                        // uint32_t power =1;
                        // while(read_adjust_offset >>=1) power <<=1;
                        read_adjust_offset = read_adjust_offset- read_adjust_offset%8; //power;
                        printf("thread %d  %d adjust to %d\n", tasklet_id, org, read_adjust_offset);
                        break;
                    }
                    else{
                        //printf("found record\n");
                        if(record_end -record_start> MIN_RECORDS_LENGTH) {
                            printf("thread %d found record\n", tasklet_id);
                            printRecord(record_start, record_end -record_start);
                            record_count++;
                        }
                        total += record_end -record_start;
                        read_adjust_offset = BLOCK_SIZE;

                    }
                }
                while(total < BLOCK_SIZE);
            }
            star += read_adjust_offset; //- read_adjust_offset%8;
            // star = &DPU_BUFFER[start+read_adjust_offset];
            // start +=read_adjust_offset;
        }while(star < max_addr && star < end_pos);
   }

    printf("total records count %d\n", record_count);
    return true;
   
}





int main() {
    int tasklet_id = me();
    uint8_t *cache = DPU_CACHES[tasklet_id];

    // buddy_init(MAX_BODY_ALLOCTE);
    uint32_t offset = (BUFFER_SIZE-(BUFFER_SIZE%NR_TASKLETS))  / NR_TASKLETS;
    uint32_t start_index = tasklet_id * offset;


    if(tasklet_id == 0) {
        if(!parseKey()) {
            printf("dpu parse search string failed return\n");
            return 0;
        }
    }

    if(!parseJson(start_index, offset, cache) ){
        return 0;
    }

}

    

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
uint8_t __mram_ptr* star[NR_TASKLETS];
uint8_t __mram_ptr* end_pos[NR_TASKLETS];


#define DEBUG

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
    volatile int tasklet_id = me();
    star[tasklet_id] = &(DPU_BUFFER[start]);
    uint8_t __mram_ptr * max_addr =  &DPU_BUFFER[BUFFER_SIZE-1];
    
    printf("thread%d start position %x  max addr %x offset %d\n", tasklet_id, (uintptr_t)star[tasklet_id], (uintptr_t) max_addr, offset);

    mram_read(star[tasklet_id], cache, BLOCK_SIZE);
    uint32_t read_adjust_offset = BLOCK_SIZE;

    if(tasklet_id !=0) {
        int copy_size = BLOCK_SIZE;
        mram_read(star[tasklet_id], cache, BLOCK_SIZE);
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
            star[tasklet_id] = &DPU_BUFFER[start+valid_start - cache +1];
        }

        printf("thread %d actuay start %x\n", tasklet_id, (uintptr_t)star[tasklet_id]);
    }


   uint8_t* record_end;
   uint32_t record_count =0;
   uint8_t __mram_ptr* thread_org_start = NULL;

        if(tasklet_id == (NR_TASKLETS-1) ){
            end_pos[tasklet_id] = max_addr;
            thread_org_start = star[tasklet_id];
            //return true;
        }
        else {


        end_pos[tasklet_id] = star[tasklet_id]+ offset-1;  /* might get lucky and end up perfectly aligned, so check that char */
        }
        printf("thread%d end position %x\n", tasklet_id, (uintptr_t)(end_pos[tasklet_id]));
//        uint32_t count = 0;
        do {
            mram_read(star[tasklet_id], cache, BLOCK_SIZE);
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
                    //printRecord(cache, record_length);
                    //printf("thread %d found record length%d first if\n", tasklet_id, record_length);
                    printf("CHECK thread %d record id: %c%c%c%c%c first if\n", tasklet_id, *(cache+20), *(cache+21), *(cache+22),*(cache+23),*(cache+24));
                    //record_count++;
                    printf("thread %d %c%c%c%c%c%c%c\n", tasklet_id, cache[0], cache[1], cache[2], cache[3], cache[4], cache[5],cache[6]);

                }
                // printRecord(cache, record_length);

                uint32_t total = record_length;
                uint32_t max_block_size = BLOCK_SIZE;
                // search within cache for another records   
                if(tasklet_id == (NR_TASKLETS-1)){
                    max_block_size = max_addr - thread_org_start;
                }              
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
                        read_adjust_offset = read_adjust_offset- read_adjust_offset%8; 
                        printf("thread %d  %d adjust to %d\n", tasklet_id, org, read_adjust_offset);
                        if(tasklet_id == (NR_TASKLETS-1)) {
                            if(read_adjust_offset == 0 ){
                                read_adjust_offset = BLOCK_SIZE;
                                printf("max_block_size %x  total %d, start \n", max_block_size, total);
                                printf("%s\n", record_start);
                                break;
                            }
                        }
                        break;
                    }
                    else{
                        //printf("found record\n");
                        if(record_end -record_start> MIN_RECORDS_LENGTH) {
                            printf("thread %d found record length %d\n", tasklet_id, record_end -record_start);
                            printf("CHECK thread %d record ID: %c%c%c%c%c searching\n", tasklet_id, *(record_start+20), *(record_start+21), *(record_start+22),*(record_start+23),*(record_start+24));
                            // printRecord(record_start, record_end -record_start);
                            record_count++;
                        }
                        total += record_end -record_start;
                        read_adjust_offset = BLOCK_SIZE;

                    }
                }
                while(total < max_block_size);
            }
            star[tasklet_id] += read_adjust_offset; //- read_adjust_offset%8;
 
            // if(tasklet_id == (NR_TASKLETS-1)) {
            

            //     if(count> 1){
            //         break;
            //     }
            //     else{
            //         count++;
            //     }
                
            // }
        }while(star[tasklet_id] < max_addr && star[tasklet_id] < end_pos[tasklet_id]);
   

    printf("thread %d star finished at %x total records count %d\n", tasklet_id, (uintptr_t)star[tasklet_id],record_count);
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

    

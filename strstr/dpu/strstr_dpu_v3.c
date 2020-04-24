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


#define MAX_BODY_ALLOCTE 4096
#define MIN_RECORDS_LENGTH 8

__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t WRITE_CACHE[BLOCK_SIZE];
__dma_aligned uint8_t key_cache[MAX_KEY_SIZE];
//__dma_aligned uint8_t STR_CACHE[BLOCK_SIZE];
__mram_noinit uint8_t DPU_BUFFER[BUFFER_SIZE];
__mram_noinit uint8_t RECORDS_BUFFER[RETURN_RECORDS_SIZE];
__mram_noinit uint8_t KEY[MAX_KEY_SIZE];
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];
uint8_t __mram_ptr* star[NR_TASKLETS];
uint8_t __mram_ptr* end_pos[NR_TASKLETS];
__mram_noinit uint32_t RECORDS_LENGTH=0;
//uint8_t __mram_ptr* ret = &(RECORDS_BUFFER[0]);
const char *sw_line = "\n\n\n\n\n\n\n";
#define DEBUG
//fsb_allocator_t allocator;
// allocator = fsb_alloc(sizeof(uint8_t), BLOCK_SIZE);

// static void initialize_allocator() {
//   allocator = fsb_alloc(sizeof(uint8_t), BLOCK_SIZE);
// }
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

/* trick to use system strstr for string filtering */
bool strstr_org(uint8_t* record_start, uint32_t length) {

#if 1    
    record_start[length] = '\0';
    if(!strstr((char*)record_start, (char*)key_cache)) {
        record_start[length] = '\n';
        return false;
    }
    //printRecord(record_start, length);
    record_start[length] = '\n';
    return true;
#else
    record_start = NULL;
    length = 0;
    return false;
#endif
}



uint32_t roundDown8(uint32_t a){
    return a-a%8;
}

uint32_t roundUp8(uint32_t a) {
    return a+8-a%8;
}

/* write records to mram */
void writeBackRecords(uint8_t* record_start, uint32_t length) {
    //TODO
    // uint8_t *ptr = (uint8_t*)roundUp8((uintptr_t)&(WRITE_CACHE[0]));
    // memcpy(ptr, record_start, length+1);
    // ptr[length+2] = '\n';

    // uint8_t * data = fsb_get(allocator);
    // uint8_t *ptr = (uint8_t*)roundUp8((uintptr_t)data);

    // lazy way to solve the src addr has to be 8 byte aligned
    // memcpy(ptr, record_start, length+1);
    // ptr[length+1] = '\n';

    uint32_t temp = RECORDS_LENGTH;
    RECORDS_LENGTH +=  roundUp8(length+2);
    // uint8_t* adjust_star = (uint8_t*)roundDown8((uintptr_t)record_start);
    mram_write(record_start, &RECORDS_BUFFER[temp], roundUp8(length+1));
    // fsb_free(allocator, data);
    return;
}


bool parseJson(uint32_t start, uint32_t offset, uint8_t* cache) {
    volatile int tasklet_id = me();
    star[tasklet_id] = &(DPU_BUFFER[start]);
    uint8_t __mram_ptr * max_addr =  &DPU_BUFFER[BUFFER_SIZE-1];
    uint32_t initial_offset = 0;

    mram_read(star[tasklet_id], cache, BLOCK_SIZE);


    /* go to the first \n */
    if(tasklet_id !=0) {
        int copy_size = BLOCK_SIZE;
        mram_read(star[tasklet_id], cache, BLOCK_SIZE);
        // in case we read over
        if(&DPU_BUFFER[start+BLOCK_SIZE] >= max_addr) {
            copy_size = BUFFER_SIZE - start;
        }
        
        uint8_t* valid_start  = (uint8_t *)memchr(cache, '\n', copy_size);

        if(valid_start == NULL) {
            //printf("thread %d could not find newline char returning early\n", tasklet_id);
            return false;
        }
        else {
            // star[tasklet_id] = &(DPU_BUFFER[start+(valid_start - cache) +1]);
            // mram_start+=valid_start - cache +1;
            // if(((uintptr_t)star[tasklet_id])%8!=0){
            //                         printf("not 8 byte aligned 3\n");
            //     }
            initial_offset = valid_start - cache +1;
            //printf("task %d init offset val is %d\n", tasklet_id, initial_offset);
        }
    }


    uint8_t __mram_ptr* thread_org_start = NULL;


    /* check end position */
    if(tasklet_id == (NR_TASKLETS-1) ){
            end_pos[tasklet_id] = max_addr;
            thread_org_start = star[tasklet_id]+ initial_offset; // TBD ERR
            //return true;
    }
    else {
        end_pos[tasklet_id] = star[tasklet_id]+ offset-1; // round up? TBD
    }
#if 1
    uint8_t* record_start;
    uint8_t* record_end;
    uint32_t record_count =0;
    uint32_t mram_start = start; // TBD
    uint32_t read_adjust_offset = BLOCK_SIZE;
    /* start parsing */
    do {
        mram_read(star[tasklet_id], cache, BLOCK_SIZE);
        if(((uintptr_t)star[tasklet_id])%8!=0) {printf("not 8 byte aligned 1 %d\n", ((uintptr_t)star[tasklet_id])); return false;}
        /* adjust */
        record_start = cache;
        if(initial_offset != 0) {
            record_start += initial_offset;
            //initial_offset =0
        }
        // else {
        //     record_start = cache;
        // }

        record_end = (uint8_t*) memchr(record_start, '\n', BLOCK_SIZE-initial_offset); // maybe need - ini_offset
        uint8_t* block_end = cache+BLOCK_SIZE-1;

        if(!record_end) {
            //printf("thread %d could not find newline char returning early stage 2\n", tasklet_id);
            return false;
        } 
        else {
            uint32_t record_length = record_end - record_start;
            if(record_length < MIN_RECORDS_LENGTH){
                //printf("thread %d address shift\n", tasklet_id);
            }
            else {
                // found a record
                record_count++;
                if(strstr_org(record_start, record_length)) {
                    // call writeback records TODO
                    writeBackRecords(record_start, record_length);
                    printf("strstr\n");
                }
            }

            uint32_t total = record_length+1;
            uint32_t max_block_size = BLOCK_SIZE;
            if(tasklet_id == (NR_TASKLETS-1)){
                max_block_size = max_addr - thread_org_start;
            }

            do {
                record_start = (uint8_t*)(record_end+1);
                if(record_start >block_end) {
                    read_adjust_offset = BLOCK_SIZE;
                    break;
                }

                record_end = (uint8_t*)memchr(record_start, '\n', BLOCK_SIZE-total-initial_offset);             
                // initial_offset=0;
                if(!record_end) {
                    read_adjust_offset = record_start-cache;
                    read_adjust_offset = read_adjust_offset- read_adjust_offset%8; 
                    if(tasklet_id == (NR_TASKLETS-1)) {
                        if(read_adjust_offset == 0 ){
                            // TBD
                            read_adjust_offset = BLOCK_SIZE;
                            break;
                        }
                    }
                    break;
                }
                else {
                    if(record_end -record_start> MIN_RECORDS_LENGTH) {
                        if(strstr_org(record_start, (record_end -record_start))) {
                            writeBackRecords(record_start, (record_end -record_start));
                            printf("second strstr \n");
                        }
                        record_count++;
                    }

                    total += record_end - record_start+1;
                    read_adjust_offset = BLOCK_SIZE;
                }
            //break;
            }while(total < max_block_size);
        }
        // mram 8 byte aligned + 8 byte aligned 
        mram_start += read_adjust_offset;
        star[tasklet_id] = &(DPU_BUFFER[mram_start]);
        initial_offset=0;
    }while(star[tasklet_id] < max_addr && star[tasklet_id] < end_pos[tasklet_id]);
#endif
    return true;

}





int main() {
    int tasklet_id = me();
    uint8_t *cache = DPU_CACHES[tasklet_id];
    uint32_t offset = (BUFFER_SIZE-(BUFFER_SIZE%NR_TASKLETS))  / NR_TASKLETS;
    uint32_t start_index = roundDown8(tasklet_id * offset);
    

    if(tasklet_id == 0) {
        if(!parseKey()) {
            printf("dpu parse search string failed return\n");
            return 0;
        }
        //initialize_allocator();
    }

    if(!parseJson(start_index, offset, cache) ){
        return 0;
    }

    printf("dpu done\n");
}
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

/* dpu marcos */
#define BLOCK_SIZE 2048 
#define MAX_BODY_ALLOCTE 4096
#define MIN_RECORDS_LENGTH 8
#define STRSTR strstr_org

/* global variables */
__host uint32_t input_length = 0;
__host uint32_t output_length = 0;
__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t key_cache[MAX_KEY_SIZE];
unsigned int key = 0x0;
__host __mram_ptr uint8_t *DPU_BUFFER;
__host __mram_ptr uint8_t *RECORDS_BUFFER;
__mram_noinit uint8_t KEY[MAX_KEY_SIZE];
uint8_t __mram_ptr* star[NR_TASKLETS];
uint8_t __mram_ptr* end_pos[NR_TASKLETS];



void shift32(uint8_t* start, unsigned int *a){
    *a = 0;
    *a |= ((start[0] & 0xFF) << 24);
    *a |= ((start[1] & 0xFF) << 16);
    *a |= ((start[2] & 0xFF) << 8);
    *a |= ((start[3] & 0xFF));
}

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
        shift32((uint8_t*)key_cache, &key);
        printf("valid key 0x%x\n", key);
        return true;
    }
    else {
        printf("invalid key\n");
        return false;
    }
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

/* helper method */
uint32_t roundDown8(uint32_t a){
    return a-a%8;
}

uint32_t roundUp8(uint32_t a) {
    return a+8-a%8;
}

/* 
 * Write records back to mram
 */
void writeBackRecords(uint8_t* record_start, uint32_t length) {
    uint32_t temp = output_length;
    output_length +=  roundUp8(length+(uintptr_t)record_start%8);
    mram_write(record_start-(uintptr_t)record_start%8, RECORDS_BUFFER+temp, roundUp8(length+(uintptr_t)record_start%8));//roundUp8(length+1));
    //printRecord(record_start, length);
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



bool strstr_comb4(uint8_t* record_start, uint32_t length) {
    int i =0;
    unsigned int res;
    unsigned int a = 0;

    do {
        if(length -i < 4) {
            break;
        }

        // unsigned int b = 0;
        shift32(record_start+i, &a);
        // printf("a 0x%x %c%c%c%c\n", a, record_start[i], record_start[i+1], record_start[i+2], record_start[i+3]);
        // printf("b 0x%x\n", key);

        __builtin_cmpb4_rrr(res, a, key);  
        // printf("result 0x%x\n", res);
        if((res^0x01010101) == 0) {
            return true;
        }  
        i += 1;
    } while(i< (int)length);

    return false;
}


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



bool strstr_comb4_op(uint8_t* record_start, uint32_t length) {
    int i = 0;
    int j = 0; 
    unsigned int res = 0;
    unsigned int a = 0;
    unsigned int b = 0;
    int next =0;

    do {
        if(length -i < 4) {
            break;
        }
        for(j=0; j< 4; j++) {
            shift32(record_start+i, &a);
            shift_same(key_cache+j, &b);
            __builtin_cmpb4_rrr(res, a, b);
            // jth byte matches
            if(res & (0x01<<((3-j)*8))){
                continue;
            }
            else {
                next = find_next_set_bit(res, j);
                break;
            }
        }
        if (j==4) {
            return true;
        }
        i += next;
    } while (i < (int) length);

    return false;
}




bool parseJson(uint32_t start, uint32_t offset, uint8_t* cache) {
    volatile int tasklet_id = me();
    star[tasklet_id] = &(DPU_BUFFER[start]);
    uint8_t __mram_ptr * max_addr =  &DPU_BUFFER[BUFFER_SIZE-1];
    uint32_t initial_offset = 0;

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
            return false;
        }
        else {
            initial_offset = valid_start - cache +1;
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
    /* start parsing */
    do {
        uint32_t read_adjust_offset = BLOCK_SIZE;
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
            if(record_length > MIN_RECORDS_LENGTH){
                // found a record
                record_count++;
                if(STRSTR(record_start, record_length)) {
                    // call writeback records TODO
                    //printf("strstr\n");
                    writeBackRecords(record_start, record_length);
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

                    if(read_adjust_offset == 0 ){
                        // TBD
                        read_adjust_offset = BLOCK_SIZE;
                        break;
                    }
                    
                    break;
                }
                else {
                    if(record_end -record_start> MIN_RECORDS_LENGTH) {
                        if(STRSTR(record_start, (record_end -record_start))) {
                            //printf("second strstr tasklet %d\n", me());
                            writeBackRecords(record_start, (record_end -record_start));
                            
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
        if(tasklet_id == NR_TASKLETS-1) {
            // printf("tasklet 4 address at %x\n", (uintptr_t)(star[tasklet_id]));
            // printf("\n");
            if(star[tasklet_id]+BLOCK_SIZE/2 > end_pos[tasklet_id]){
                break;
            }
        }
    } while(star[tasklet_id] < max_addr && star[tasklet_id] < end_pos[tasklet_id]);
#endif
    return true;

}




int main() {
    int tasklet_id = me();
#if 1
    uint8_t *cache = DPU_CACHES[tasklet_id];
    uint32_t offset = (BUFFER_SIZE-(BUFFER_SIZE%NR_TASKLETS))  / NR_TASKLETS;
    uint32_t start_index = roundDown8(tasklet_id * offset);
#endif    
    if(input_length == 0) {
        return 0;
    }
    //printf("tasklet %d is running\n", tasklet_id);

    if(tasklet_id == 0) {
        if(!parseKey()) {
            printf("dpu parse search string failed return\n");
            return 0;
        }
        //printf("input %x write back %x\n", (uintptr_t)DPU_BUFFER, (uintptr_t)RECORDS_BUFFER);
    }
#if 0
    char buf[20] = "abcaabadef";
    if(strstr_comb4_op((uint8_t*)buf, strlen(buf))) {
        printf("strstr true\n");
    }
    else{ 
        printf("strstr false\n");
    }
#endif
#if 1
    if(!parseJson(start_index, offset, cache) ){
        return 0;
    }
#endif
    //printf("dpu done\n");
}
#include <defs.h>
#include <mram.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <perfcounter.h>
#include <string.h>
#include <mutex.h>
#include <handshake.h>
#include <alloc.h>
#include <sem.h>

static const char * mutex_name[] = {"task_1", "task_2",
                                  "task_3",  "task_4", NULL}; 
char str[] ="There are only a few changes to the setup in the paper:\nWe train less steps (we do apple steps, the paper does 450k steps), but this is configurable.\nThe branches for the controller follow the order of the training data.\nWe apple different weight hyperparameters for the outputs (steer, gas, brake, speed),\nsince the hyperparameters suggested in the paper did not work for us.\n";

char search_str[] ="apple";

typedef struct{
    uint8_t* cache;
    mutex_id_t mutex_id;
    bool copied;
    int record_offset;
}dpu_block_t;


#ifndef  NR_TASKLETS
    #define NR_TASKLETS 3
#endif


#define BLOCK_SIZE 512
#define MAX_RECORD 512

__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS-1][BLOCK_SIZE];
__dma_aligned int DPU_RESULTS[NR_TASKLETS-1][MAX_RECORD];
__dma_aligned int tasklet_record_process_count[NR_TASKLETS-1];

int record_count = 0;
int record_processed = 0;
bool done = false;
dpu_block_t caches[NR_TASKLETS-1];

MUTEX_INIT(1);
MUTEX_INIT(2);
MUTEX_INIT(3);
MUTEX_INIT(4);

MUTEX_INIT(PROC_COUNT);
SEMAPHORE_INIT(done_sem, 0);




// semaphor used for termination 
// 


void initialize_dpu_block(dpu_block_t *caches) {
    for(int i=0; i< NR_TASKLETS-1; i++){
        caches[i].cache = DPU_CACHES[i];
        caches[i].copied = false;
    }
    caches[0].mutex_id = MUTEX_GET(1);
    caches[1].mutex_id = MUTEX_GET(2);
    caches[2].mutex_id = MUTEX_GET(3);
    caches[3].mutex_id = MUTEX_GET(4);


}


int get_block_free(dpu_block_t *caches) {
    // loop through caches to find a vacancy block
    while(true){
        for(int i=0; i<NR_TASKLETS-1; i++) {
            if(mutex_trylock(caches[i].mutex_id)) {
                if(caches[i].copied == false){
                    return i;
                }     
            }
        }
    }

}



/*
 * parse the json records to mram cache
 */
int parse() {
    buddy_init(256);
    uint8_t *substr = buddy_alloc(256);
    int record_count =0;
    int offset =0;
    int copy_size = 256;
    long total_records_len = 0;
    sem_id_t sem_id = SEMAPHORE_GET(done_sem);

    uint8_t* current_record_start;
    uint8_t* current_record_end;
    uint8_t* input_last_byte;
    mutex_id_t process_count_lock = MUTEX_GET(PROC_COUNT);
    // dpu_block_t caches[NR_TASKLETS-1];

    initialize_dpu_block(caches);
    long str_len = strlen(str);

    for(int i=0; i< str_len; i +=offset){
        // simulate MRAMREAD
        if(i+offset > str_len) {
            copy_size = str_len -i;
        }
        (void)memcpy(substr, str+i, copy_size);
        // find all the records
        current_record_start = (uint8_t*)substr;
        // if(input_last_byte < )
        input_last_byte = current_record_start+256-1;

        // + records * '\n'
        while(current_record_start < input_last_byte && (total_records_len)+record_count < str_len) {
            current_record_end = (uint8_t *)memchr(current_record_start, '\n',
            input_last_byte - current_record_start);

            if(!current_record_end){
                offset = current_record_start- substr;
                break;
            }
            // at this point we found a record
            record_count++;
            size_t record_length = current_record_end - current_record_start;

            // 
            int free_block = get_block_free(caches);
            // copy records into the cache
            memcpy(caches[free_block].cache, current_record_start, record_length);
            caches[free_block].copied = true;
            caches[free_block].record_offset = total_records_len+1;

            // free the lock 
            mutex_unlock(caches[free_block].mutex_id);
            // wake up the worker
            // handshake_notify();
            total_records_len += record_length;
            current_record_start = current_record_end+1;
            offset = 256;
        }
    }

    // find all the records
    // semaphore
    // lock done flag
    mutex_lock(process_count_lock);
    done =true;
    mutex_unlock(process_count_lock);
    buddy_free(substr);

    for(int t=0; t< NR_TASKLETS-1; t++) {
        sem_take(sem_id);
    }

    return 0;
}

int process() {
        // lock the done flag 
        // if done break 

        // after finishing process a record => call semaphore
    sem_id_t sem_id = SEMAPHORE_GET(done_sem);
    while(true) {
        int task_id = me()-1;
        mutex_id_t process_count_lock = MUTEX_GET(PROC_COUNT);


        if(!caches[task_id].cache){
            continue;
        }

        // mutex_lock(process_count_lock);
        // if(done){
        //     // all records has been processed finish
        //     mutex_unlock(process_count_lock);
        //     break;
        // }
        // mutex_unlock(process_count_lock);



        mutex_lock(caches[task_id].mutex_id);
        mutex_lock(process_count_lock);


        if(!caches[task_id].copied) {
            if(done){
                // all records has been processed finish
                mutex_unlock(process_count_lock);
                mutex_unlock(caches[task_id].mutex_id);
                break;
            }

            mutex_unlock(process_count_lock);
            mutex_unlock(caches[task_id].mutex_id);
            continue;
        }
        mutex_unlock(process_count_lock);

        if(strstr((char*)caches[task_id].cache, search_str)) {
            
            int index = tasklet_record_process_count[task_id];
            DPU_RESULTS[task_id][index] = caches[task_id].record_offset;
            tasklet_record_process_count[task_id] +=1;
        }

        caches[task_id].copied = false;

        mutex_unlock(caches[task_id].mutex_id);


    }

    sem_give(sem_id);
    return 0;
}

int main() {
    return me() ==0 ? parse(): process();
}


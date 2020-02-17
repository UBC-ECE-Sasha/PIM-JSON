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

//#define BLOCK_SIZE 256
#define BLOCK_SIZE 1024
__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];

__mram_noinit uint8_t DPU_BUFFER[BUFFER_SIZE];
//MUTEX_INIT(strstr_mutex);

//int found = 0;

int main() {
  int tasklet_id = me();
  uint8_t *cache = DPU_CACHES[tasklet_id];
  dpu_results_t *result = &DPU_RESULTS[tasklet_id];
//  mutex_id_t mutex_id = MUTEX_GET(strstr_mutex);

  /* Initialize once the cycle counter */
  if (tasklet_id == 0)
      perfcounter_config(COUNT_CYCLES, true);

  // mutex_lock(mutex_id);
  // if(found == 1){
  //   return 0;
  // }
  // mutex_unlock(mutex_id);

  unsigned char str[4] = {'B', 'a', 'b', 'y'};
  int offset = strlen((char*)str);
  uint32_t buffer_idx = (tasklet_id==0) ? 0 : (tasklet_id * BLOCK_SIZE-offset);

  for (; buffer_idx < BUFFER_SIZE;
    buffer_idx += (NR_TASKLETS * BLOCK_SIZE)) {

    /* Load cache with current MRAM block. */
    MRAM_READ((mram_addr_t)&DPU_BUFFER[buffer_idx], cache, BLOCK_SIZE);

    unsigned char* s1;
    //nsigned char str[3] = {'T', 'h', 'e'};
    // Baby
    unsigned char*cp = (unsigned char*)cache;
    for (int i =0; i< BLOCK_SIZE; i++) {
      s1= cp;

      unsigned char* s2= str;
      while(*s1 &&*s2&&!(*s1-*s2)){
        s1++; 
        s2++;
      }

      if(!*s2) {
        result->found = 1;
        // mutex_lock(mutex_id);
        // found = 1;
        // mutex_unlock(mutex_id);        
        break;
        // return;
      } 

      cp++;
    }
  }

    /* keep the 32-bit LSB on the 64-bit cycle counter */
    result->cycles = (uint32_t)perfcounter_get();
    // result->checksum = checksum;

    printf("[%02d] string found = 0x%08x\n", tasklet_id, result->found);
    return 0;


}
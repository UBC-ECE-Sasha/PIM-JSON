/* Communication with a DPU via the MRAM. */
/* Populate the MRAM with a collection of bytes and request the DPU to */
/* compute the checksum. */

#include <dpu.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include <dpu_log.h>
#include <time.h>
#include <string.h>

#ifndef DPU_BINARY
#define DPU_BINARY "build/strstr_dpu"
#endif

#define DPU_LOG_ENABLE
#define CYCLES 500000000

char* read_json_file() {
    char *source = NULL;
    FILE *fp = fopen("tweets_sm.json", "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */ }

            /* Allocate our buffer to that size. */
            printf("buffer size is %ld\n", bufsize);
            source = malloc(sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if ( ferror( fp ) != 0 ) {
                fputs("Error reading file", stderr);
            } else {
                source[newLen++] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
    }
    char *json_source = malloc(sizeof(char) * (BUFFER_SIZE + 1));
    strncpy(json_source, source, BUFFER_SIZE);
    // printf("%s", json_source);
    //free(source); 
    //printf("%s\n", json_source+(BUFFER_SIZE-50));
    return json_source;  
}

char* populate_mram(struct dpu_set_t set) {
  int byte_index;
  uint8_t buffer[BUFFER_SIZE];
  printf("populating data\n");
  // DPU_ASSERT(dpu_copy_to(set, "buffer_size_input", 0, (const uint8_t *)&nr_bytes, sizeof(nr_bytes)));
  char* json_str = read_json_file();
  printf("finish reading data\n");

  for (byte_index = 0; byte_index < BUFFER_SIZE; byte_index++) {
    buffer[byte_index] = (uint8_t)json_str[byte_index];
  }
  DPU_ASSERT(dpu_copy_to(set, XSTR(DPU_BUFFER), 0, buffer, BUFFER_SIZE));

  uint8_t key_buffer[MAX_KEY_SIZE];

  char key[] ="rump\n";
  int len = strlen(key);
  for (byte_index = 0; byte_index < len; byte_index++) {
    key_buffer[byte_index] = (uint8_t)key[byte_index];
  }

  DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, key_buffer, MAX_KEY_SIZE));


  return json_str;
 }

int host_strstr(char* json_str) {
  char*s1,*s2;
  char*cp = json_str;

  for (uint32_t i =0; i< strlen(json_str); i++) {
    s1= cp;
    s2 = (char*)"Baby";
    while(*s1 &&*s2&&!(*s1-*s2)){
      s1++; 
      s2++;
    }

    if(!*s2) {

      return 1;
    } 

    cp++;
  }

  return 0;  
}


int main() {
  struct dpu_set_t set, dpu;
  uint32_t found;
  printf("populating data");
  DPU_ASSERT(dpu_alloc(1, NULL, &set));
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
  char* json_str = populate_mram(set);
  uint32_t dpu_cycles=800000000;
  clock_t start, end;

  DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

#ifdef DPU_LOG_ENABLE 
  {
    unsigned int each_dpu = 0;
    printf("Display DPU Logs\n");
    DPU_FOREACH (set, dpu) {
      printf("DPU#%d:\n", each_dpu);
      DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
      each_dpu++;
    }
  }
#endif

  for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++) {
    dpu_results_t result;
    DPU_ASSERT(dpu_copy_from(dpu, XSTR(DPU_RESULTS), each_tasklet * sizeof(dpu_results_t), &result, sizeof(dpu_results_t)));

    if(result.found == 1) {
      found = 1;
    }
    if (result.cycles < dpu_cycles)
        dpu_cycles = result.cycles;
  }
  
  start = clock();
  int host_result = host_strstr(json_str);
  end = clock();

  double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("DPU execution time  = %g cycles  %g seconds\n", (double)dpu_cycles, (double)dpu_cycles/CYCLES);
  printf("performance         = %g cycles/byte\n", (double)dpu_cycles / BUFFER_SIZE);
  printf("strstr %d\n", found);



  printf("host result %d, time used by host %g \n", host_result, cpu_time_used);


  DPU_ASSERT(dpu_free(set));
  return 0;
}
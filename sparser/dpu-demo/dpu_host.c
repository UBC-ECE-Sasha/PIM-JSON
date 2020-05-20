#include <dpu.h>
#include <dpu_log.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "dpu_host.h"
#include "dpu_common.h"
// dpu binary location TBD
#define DPU_BINARY "build/sparser_dpu"
#define DPU_LOG_ENABLE 1
#define WRITE_OUT 1

void dpu_test(char *input, char* key, char* ret) {


	struct dpu_set_t set, dpu;
    DPU_ASSERT(dpu_alloc(1, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
    // key = NULL;
    // input = NULL;
    // printf("%c", input[0]);
    printf("%c", key[0]);
	//copy data to DPU
    clock_t start, end;
    start = clock();
	DPU_ASSERT(dpu_copy_to(set, XSTR(DPU_BUFFER), 0, (unsigned char*)input, BUFFER_SIZE));
	//copy key
    DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, (unsigned char*)"aabaa\n", MAX_KEY_SIZE));
    end = clock();
    printf("copy data to dpu took %g s\n",((double) (end - start)) / CLOCKS_PER_SEC);
	// launch DPU
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    

#ifdef DPU_LOG_ENABLE 
    {
        unsigned int each_dpu = 0;
        printf("Display DPU Logs\n");
        DPU_FOREACH (set, dpu) {
        printf("DPU#%d:\n", each_dpu);
        // DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
        each_dpu++;
        }
    }
#endif 

#if WRITE_OUT

    uint32_t records_len=0;

    start = clock();
    DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_LENGTH), 0, (uint8_t*)&records_len, sizeof(records_len)));
    DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_BUFFER), 0, (uint8_t*)ret, RETURN_RECORDS_SIZE));
    end = clock();
    printf("records length is %d took %g s\n", records_len, ((double) (end - start)) / CLOCKS_PER_SEC);
    printf("\n");
#endif
}
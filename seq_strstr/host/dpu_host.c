#include <dpu.h>
#include <dpu_log.h>
#include <dpu_memory.h>
#include <dpu_program.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "dpu_host.h"
#include "common.h"
// dpu binary location TBD
#define DPU_BINARY "build/strstr_dpu"
#define DPU_LOG_ENABLE 1
#define WRITE_OUT 1
#define ALIGN(_p, _width) (((unsigned int)_p + (_width-1)) & (0-_width))



dpu_error_t dpu_copy_to_dpu(struct dpu_set_t dpu, const char *symbol_name, uint32_t symbol_offset, const void *src, size_t length){
    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol;
  
    program = dpu_get_program((dpu.dpu));
    if ((status = dpu_get_symbol(program, symbol_name, &symbol)) != DPU_OK) {
        return status;
    }

    return dpu_copy_to_symbol_dpu(dpu.dpu, symbol, symbol_offset, src, length);
}


dpu_error_t dpu_copy_from_dpu(struct dpu_set_t dpu, const char *symbol_name, uint32_t symbol_offset, void *dst, size_t length){
    dpu_error_t status;
    struct dpu_program_t *program;
    struct dpu_symbol_t symbol;
  
    program = dpu_get_program((dpu.dpu));
    if ((status = dpu_get_symbol(program, symbol_name, &symbol)) != DPU_OK) {
        return status;
    }

    return dpu_copy_from_symbol_dpu(dpu.dpu, symbol, symbol_offset, dst, length);
}

void printRecord(char* record_start, uint32_t length) {
    for(uint32_t i =0; i< length; i++){
        printf("%c", record_start[i]);
    }

    printf("\n");
    printf("\n");
}



#define ALIGN(_p, _width) (((unsigned int)_p + (_width-1)) & (0-_width))
// give a large file we need to divide the file into proper chunks for each dpu and each tasklet
bool calculate_offset(char *input, long length, uint32_t input_offset[NR_DPUS][NR_TASKLETS]) {
    length = 1<<20;
    uint32_t block_size = (ALIGN(length, 8)) / (NR_DPUS * NR_TASKLETS);

    printf("%c\n", input[0]);
    if(block_size < 64) {
        return false;
    }
    int dpu_indx = 0;
    int tasklet_index = 0;

    for (int i=0; i< NR_DPUS * NR_TASKLETS; i++) {
        input_offset[dpu_indx][tasklet_index] = i * block_size;

        if(tasklet_index == NR_TASKLETS-1) {
            dpu_indx++;
            tasklet_index =0;
        }
        else {
            tasklet_index++;
        }
    }
    return true;
}


void multi_dpu_test(char *input, long length, uint8_t** ret, uint32_t *records_len){
    struct dpu_set_t set, dpu;
    uint32_t nr_of_dpus;
    uint32_t nr_of_ranks;
    length = 1<<20;
    
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
    DPU_ASSERT(dpu_get_nr_dpus(set, &nr_of_dpus));
    DPU_ASSERT(dpu_get_nr_ranks(set, &nr_of_ranks));
    printf("Allocated %d DPU(s) %c number of dpu ranks are %d\n", nr_of_dpus, input[0], nr_of_ranks);
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    //long offset = 0;
    unsigned int dpu_id = 0;
    // char* curr_start = input;
    // char* input_end = input+length;
    //dpu_error_t status;
    uint32_t input_length = (ALIGN(length, 8) / (NR_DPUS));
    printf("total record length is %ld\n", length);
    uint32_t dpu_mram_buffer_start = 1024 * 1024;
    uint32_t dpu_mram_ret_buffer_start = ALIGN(dpu_mram_buffer_start + BUFFER_SIZE + 64, 64);

    // copy the key in for all DPUs - hardcoded now
    // clock_t start, end;
    // long copied_length =0;
    // double duration = 0.0;
    struct timeval start;
	struct timeval end;

    uint32_t input_offset[NR_DPUS][NR_TASKLETS] = {0};
    calculate_offset(input, length, input_offset);

    gettimeofday(&start, NULL);
    for(int i=0; i< NR_DPUS; i++) {
        for (int j=0; j< NR_TASKLETS; j++) {
            printf("%d ", input_offset[i][j]);
        }
        printf("\n");
    }

    DPU_FOREACH (set, dpu) {
        DPU_ASSERT(dpu_copy_to(dpu, XSTR(KEY), 0, (unsigned char*)"rump\n", MAX_KEY_SIZE));
        DPU_ASSERT(dpu_copy_to(dpu, XSTR(DPU_BUFFER), 0, &dpu_mram_buffer_start, sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_to(dpu, XSTR(RECORDS_BUFFER), 0, &dpu_mram_ret_buffer_start, sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_to(dpu, "input_offset", 0, input_offset[dpu_id], sizeof(uint32_t) * NR_TASKLETS));
        DPU_ASSERT(dpu_copy_to(dpu, "input_length", 0, &input_length, sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_to_mram(dpu.dpu, dpu_mram_buffer_start, (unsigned char*)input, input_length, DPU_PRIMARY_MRAM));
        dpu_id++;
    }

	gettimeofday(&end, NULL);
    double start_time = start.tv_sec + start.tv_usec / 1000000.0;
	double end_time = end.tv_sec + end.tv_usec / 1000000.0;

    // printf("host %d took %g s for coping %d total copied %ld\n", dpu_id, end_time - start_time, BUFFER_SIZE, copied_length);

    gettimeofday(&start, NULL);
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    gettimeofday(&end, NULL);
    start_time = start.tv_sec + start.tv_usec / 1000000.0;
	end_time = end.tv_sec + end.tv_usec / 1000000.0;
    printf("dpu launch took %g s\n", end_time - start_time);

    {
        unsigned int each_dpu = 0;
        printf("Display DPU Logs\n");
        DPU_FOREACH (set, dpu) {
        printf("DPU#%d:\n", each_dpu);
        DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
        each_dpu++;
        }
    } 
    printf("%d %c\n",records_len[0], ret[0][0]);  
}
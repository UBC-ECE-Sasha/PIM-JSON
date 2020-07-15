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
#include <limits.h>

#include "dpu_host.h"
#include "../dpu_common.h"
// dpu binary location TBD
#define DPU_BINARY "build/sparser_dpu"
#define DPU_LOG_ENABLE 1
#define WRITE_OUT 1
#define ALIGN(_p, _width) (((unsigned int)_p + (_width-1)) & (0-_width))
#define ALIGN_LONG(_p, _width) (((long)_p + (_width-1)) & (0-_width))
#define HOST_DEBUG 0

/**
 * 
 * Utility functions 
 *  
 **/
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

void printRecordNoLen(char* record_start) {
    for(uint32_t i =0; i< 4096; i++){
        if(record_start[i] != '\n') {
            printf("%c", record_start[i]);
        }
        else {
            break;
        }

    }
    printf("\n");
    printf("\n");
}


// give a large file we need to divide the file into proper chunks for each dpu and each tasklet
bool calculate_offset(char *input, long length, uint64_t input_offset[NR_DPUS][NR_TASKLETS], uint32_t input_length[NR_DPUS]) {
    long dpu_blocksize = (ALIGN_LONG(length, 8)) / NR_DPUS;
    int dpu_indx = 0;
    int tasklet_index = 0;
    long o_end =0;
    char* r_end =0;
    long t_blocksize = 0;

    // each dpu
    for (dpu_indx=0; dpu_indx< NR_DPUS; dpu_indx++) {
        if(dpu_indx != NR_DPUS-1) {
            o_end = (dpu_indx+1) * dpu_blocksize;
            r_end = memchr(input+o_end, '\n', length- o_end);
            input_offset[dpu_indx+1][0] = r_end-input +1;
        #if HOST_DEBUG
            dbg_printf("dpu %d starts at %d %c\n", dpu_indx+1, input_offset[dpu_indx+1][0], input[input_offset[dpu_indx+1][0]]);
        #endif
        }
    }

    // each tasklet
    for (dpu_indx=0; dpu_indx< NR_DPUS; dpu_indx++) {
        if(dpu_indx != NR_DPUS-1) {
            input_length[dpu_indx] = input_offset[dpu_indx+1][0] - input_offset[dpu_indx][0];
        }
        else {
            input_length[dpu_indx] = length - input_offset[dpu_indx][0];
        }

        t_blocksize = input_length[dpu_indx] / NR_TASKLETS;
        for(tasklet_index = 0; tasklet_index< NR_TASKLETS; tasklet_index++) {
            if(tasklet_index != NR_TASKLETS-1) {

                o_end = (tasklet_index+1) * t_blocksize + input_offset[dpu_indx][0];
                r_end = memchr(input+o_end, '\n', length- o_end);
                input_offset[dpu_indx][tasklet_index+1] = (r_end-input +1 - (uint64_t)input_offset[dpu_indx][0]);
                #if HOST_DEBUG
                    dbg_printf("dpu %d tasklet %d starts at %d %c\n", dpu_indx, tasklet_index+1,input_offset[dpu_indx][tasklet_index+1], input[input_offset[dpu_indx][tasklet_index+1]]);
                #endif
            }

        }
    }

    return true;
}


/************************************************************************************************** 
*
*
* actual code starts here
*
*
****************************************************************************************************/
void multi_dpu_test(char *input, unsigned int * keys, uint32_t keys_length, long length, uint32_t record_offsets[NR_DPUS][NR_TASKLETS][MAX_NUM_RETURNS], uint64_t input_offset[NR_DPUS][NR_TASKLETS]){
    struct dpu_set_t set, dpu, dpu_rank;
    uint32_t nr_of_dpus;
    uint32_t nr_of_ranks;
    struct timeval start;
	struct timeval end;
        
    length = MEGABYTE(2);//4330180661;

    if(keys == NULL) {
        printf("no keys found\n");
        return;
    }
    // allocate DPUs
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));

    uint32_t dpus_per_rank = 0;
    DPU_ASSERT(dpu_get_nr_dpus(set, &nr_of_dpus));
    DPU_ASSERT(dpu_get_nr_ranks(set, &nr_of_ranks));
    dpus_per_rank = nr_of_dpus/nr_of_ranks;
    printf("Got %u dpus across %u ranks (%u dpus per rank)\n", nr_of_dpus, nr_of_ranks, dpus_per_rank);

    #if HOST_DEBUG
        dbg_printf("Allocated %d DPU(s) %c number of dpu ranks are %d\n", nr_of_dpus, input[0], nr_of_ranks);
    #endif
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    gettimeofday(&start, NULL);
    unsigned int dpu_id = 0;
    // uint32_t dpu_mram_buffer_start = 1024 * 1024;
    uint32_t input_length[NR_DPUS] = {0};

    calculate_offset(input, length, input_offset, input_length);
    gettimeofday(&end, NULL);
    double start_time = start.tv_sec + start.tv_usec / 1000000.0;
	double end_time = end.tv_sec + end.tv_usec / 1000000.0;
    printf("host preprocess took %g s \n", end_time - start_time);

    gettimeofday(&start, NULL);
    uint32_t temp_offset[NR_TASKLETS] = {0};
    uint8_t rank_id;
    UNUSED(rank_id);

    DPU_RANK_FOREACH(set, dpu_rank, rank_id)
    {
        // copy data shared among ranks
        DPU_ASSERT(dpu_copy_to(dpu_rank, "key_cache", 0, keys, sizeof(unsigned int)*keys_length));
        DPU_ASSERT(dpu_copy_to(dpu_rank, "query_count", 0, &keys_length, sizeof(uint32_t)));

        uint32_t largest_length = 0;
        DPU_FOREACH(dpu_rank, dpu)
        {
#if 1
            // input_length[dpu_id] = ALIGN(input_length[dpu_id], 8);
            for (int t=0; t<NR_TASKLETS; t++) {
                temp_offset[t] = t ==0 ? 0 :  (uint32_t)(input_offset[dpu_id][t]);
            }
#endif
            DPU_ASSERT(dpu_copy_to(dpu, "input_offset", 0, temp_offset, sizeof(uint32_t) * NR_TASKLETS));
            DPU_ASSERT(dpu_copy_to(dpu, "input_length", 0, &(input_length[dpu_id]), sizeof(uint32_t)));
            DPU_ASSERT(dpu_prepare_xfer(dpu, (void*)(input+input_offset[dpu_id][0])));
            largest_length = (input_length[dpu_id] > largest_length) ? input_length[dpu_id] : largest_length;
            dpu_id++; 
        }
        DPU_ASSERT(dpu_push_xfer(dpu_rank, DPU_XFER_TO_DPU, "dpu_mram_buffer", 0, ALIGN(largest_length, 8), DPU_XFER_DEFAULT));
    }

    
    gettimeofday(&end, NULL);
    start_time = start.tv_sec + start.tv_usec / 1000000.0;
	end_time = end.tv_sec + end.tv_usec / 1000000.0;

    printf("host took %g s for transferring memory\n", end_time - start_time);

    gettimeofday(&start, NULL);
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    gettimeofday(&end, NULL);
    start_time = start.tv_sec + start.tv_usec / 1000000.0;
	end_time = end.tv_sec + end.tv_usec / 1000000.0;
    printf("dpu launch took %g s\n", end_time - start_time);
#if HOST_DEBUG
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
    gettimeofday(&start, NULL);
    int i =0;
#if 1
    DPU_FOREACH (set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, "RECORDS_OFFSETS", 0, &(record_offsets[i]), sizeof(uint32_t) * MAX_NUM_RETURNS * NR_TASKLETS));
        i++;
    }
#endif

    gettimeofday(&end, NULL);
    start_time = start.tv_sec + start.tv_usec / 1000000.0;
	end_time = end.tv_sec + end.tv_usec / 1000000.0;    
    printf("dpu copy back records took %g s\n", end_time - start_time);
}

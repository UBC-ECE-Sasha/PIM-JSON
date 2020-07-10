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
                input_offset[dpu_indx][tasklet_index+1] = r_end-input +1;
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
void multi_dpu_test(char *input, unsigned int * keys, int keys_length, long length, uint8_t** ret, uint32_t *records_len){
    struct dpu_set_t set, dpu;
    uint32_t nr_of_dpus;
    uint32_t nr_of_ranks;
    struct timeval start;
	struct timeval end;
        
    length = MEGABYTE(2);//4330180661;

    if(keys == NULL) {
        printf("no keys found\n");
        return;
    }
    
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
    DPU_ASSERT(dpu_get_nr_dpus(set, &nr_of_dpus));
    DPU_ASSERT(dpu_get_nr_ranks(set, &nr_of_ranks));
    #if HOST_DEBUG
        dbg_printf("Allocated %d DPU(s) %c number of dpu ranks are %d\n", nr_of_dpus, input[0], nr_of_ranks);
    #endif
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    gettimeofday(&start, NULL);
    unsigned int dpu_id = 0;
    uint32_t dpu_mram_buffer_start = 1024 * 1024;
    // uint32_t dpu_mram_ret_buffer_start[NR_DPUS] = {0};
    uint64_t input_offset[NR_DPUS][NR_TASKLETS] = {0};
    uint32_t input_length[NR_DPUS] = {0};

    calculate_offset(input, length, input_offset, input_length);
    gettimeofday(&end, NULL);
    double start_time = start.tv_sec + start.tv_usec / 1000000.0;
	double end_time = end.tv_sec + end.tv_usec / 1000000.0;
    printf("host preprocess took %g s \n", end_time - start_time);

    gettimeofday(&start, NULL);
    #if HOST_DEBUG
    for(int i=0; i< NR_DPUS; i++) {
        for (int j=0; j< NR_TASKLETS; j++) {
            printf("%lu ", input_offset[i][j]);
        }
        printf("\n");
    }
    #endif
  // unsigned int key = 0x72756D70;
  //   unsigned int key = 0x61616261;

    DPU_FOREACH (set, dpu) {
        uint32_t adjust_offset = input_offset[dpu_id][0]%8;
        input_length[dpu_id] += adjust_offset;
        input_length[dpu_id] = ALIGN(input_length[dpu_id], 8);
        // dpu_mram_ret_buffer_start[dpu_id] = ALIGN(dpu_mram_buffer_start + input_length[dpu_id] + 64, 64);
        // DPU_ASSERT(dpu_copy_to(dpu, "key_cache", 0, &key, sizeof(unsigned int)));
         DPU_ASSERT(dpu_copy_to(dpu, "key_cache", 0, keys, sizeof(unsigned int)*keys_length));
        DPU_ASSERT(dpu_copy_to(dpu, XSTR(DPU_BUFFER), 0, &dpu_mram_buffer_start, sizeof(uint32_t)));
        // DPU_ASSERT(dpu_copy_to(dpu, XSTR(RECORDS_BUFFER), 0, &(dpu_mram_ret_buffer_start[dpu_id]), sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_to(dpu, "input_offset", 0, input_offset[dpu_id], sizeof(uint64_t) * NR_TASKLETS));
        DPU_ASSERT(dpu_copy_to(dpu, "input_length", 0, &(input_length[dpu_id]), sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_to(dpu, "adjust_offset", 0, &(adjust_offset), sizeof(uint32_t)));
        DPU_ASSERT(dpu_copy_to_mram(dpu.dpu, dpu_mram_buffer_start, (unsigned char*)input+input_offset[dpu_id][0]- adjust_offset, input_length[dpu_id]));
        dpu_id++;
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
#if 1
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
    uint32_t record_offsets[NR_DPUS][NR_TASKLETS][MAX_NUM_RETURNS] = {0};

#if 0
    DPU_FOREACH (set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, "output_length", 0, (uint8_t*)&(records_len[i]), sizeof(uint32_t)));
        if(records_len[i] != 0){
            DPU_ASSERT(dpu_copy_from_mram(dpu.dpu, (uint8_t*)(ret[i]), dpu_mram_ret_buffer_start[i], ALIGN(records_len[i]+16, 8), DPU_PRIMARY_MRAM));
        }
        i++;
    }
#endif
    DPU_FOREACH (set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, "RECORDS_OFFSETS", 0, &(record_offsets[i]), sizeof(uint32_t) * MAX_NUM_RETURNS * NR_TASKLETS));
        i++;
    }

    for (i =0; i< NR_DPUS; i++) {
        for (int j=0; j< NR_TASKLETS; j++) {
            char* base = input + input_offset[i][j] + input_offset[i][0]%8;
            for(int k =0; k < 16; k++) {
                printf("%u ", record_offsets[i][j][k]);

#if 0                 
                if (k == 0 && record_offsets[i][j][0] == 0 && record_offsets[i][j][1] != 0) {
                    printf("\n");
                    printRecordNoLen(input + input_offset[i][j]-input_offset[i][0] - input_offset[i][0]%8 + record_offsets[i][j][k]);
                }
                else {
                    if(record_offsets[i][j][k] != 0) {
                        printf("\n");
                        printRecordNoLen(base + record_offsets[i][j][k] + 4);
                    }
                }
#endif
#if 1
                if(record_offsets[i][j][k] != 0xDEADBAFF) {
                    printf("\n");
                    printRecordNoLen(base + record_offsets[i][j][k]);

                }else {
                    break;
                }
#endif
            }
            printf(" t ");

        }
        printf("\n");
    }

    ret[0][0] = ret[0][0];
    records_len = records_len;

    gettimeofday(&end, NULL);
    start_time = start.tv_sec + start.tv_usec / 1000000.0;
	end_time = end.tv_sec + end.tv_usec / 1000000.0;    
     printf("dpu copy back records took %g s\n", end_time - start_time);
}

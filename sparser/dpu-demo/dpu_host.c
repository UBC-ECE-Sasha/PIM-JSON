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

#include "dpu_host.h"
#include "dpu_common.h"
// dpu binary location TBD
#define DPU_BINARY "build/sparser_dpu"
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



void multi_dpu_test(char *input, long length, uint8_t** ret, uint32_t *records_len){
    struct dpu_set_t set, dpu;
    uint32_t nr_of_dpus;
    uint32_t nr_of_ranks;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
    DPU_ASSERT(dpu_get_nr_dpus(set, &nr_of_dpus));
    DPU_ASSERT(dpu_get_nr_ranks(set, &nr_of_ranks));
    printf("Allocated %d DPU(s) %c number of dpu ranks are %d\n", nr_of_dpus, input[0], nr_of_ranks);
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    long offset = 0;
    unsigned int dpu_id = 0;
    char * record_end;
    dpu_error_t status;
    printf("total record length is %ld\n", length);
    DPU_FOREACH (set, dpu) {
        if(offset + BUFFER_SIZE < length) {      
            status = dpu_copy_to_dpu(dpu, XSTR(DPU_BUFFER), 0, (unsigned char*)input+offset, BUFFER_SIZE);
            DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, (unsigned char*)"aabaa\n", MAX_KEY_SIZE));
            printf("dpu %d copy memory at offset %ld status %d\n", dpu_id, offset, status);
            offset += BUFFER_SIZE-(MAX_RECORD_SIZE/2);
            record_end = (char *)memchr(input+offset, '\n',
            length- offset);
            offset += record_end-(input+offset) +1;
            dpu_id++;
            // if(offset > length) {
            //     printf("copied overflow\n");
            //     break;
            // }
        }
        else if (offset < length) {
            status = dpu_copy_to_dpu(dpu, XSTR(DPU_BUFFER), 0, (unsigned char*)input+offset, ALIGN((length-offset), 8));
            DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, (unsigned char*)"aabaa\n", MAX_KEY_SIZE));            
        }
    }

    //DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
	int err = dpu_launch(set, DPU_SYNCHRONOUS);
	if (err != 0)
	{
		DPU_ASSERT(dpu_free(set));
		return;
	}

#ifdef DPU_LOG_ENABLE 
    // {
    //     unsigned int each_dpu = 0;
    //     printf("Display DPU Logs\n");
    //     DPU_FOREACH (set, dpu) {
    //     printf("DPU#%d:\n", each_dpu);
    //     DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
    //     each_dpu++;
    //     }
    // }
#endif 

    // uint32_t records_len[NR_DPUS];

    //start = clock();
    //DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_LENGTH), 0, (uint8_t*)&records_len, sizeof(records_len)));

    int i =0;
    DPU_FOREACH (set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_LENGTH), 0, (uint8_t*)&(records_len[i]), sizeof(uint32_t)));
        if(records_len[i] != 0){
            DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_BUFFER), 0, (uint8_t*)(ret[i]), RETURN_RECORDS_SIZE));
        }
        i++;
    }

    for(int j=0; j< NR_DPUS; j++) {
        printf("DPU %d\n found record length %d\n", j, records_len[j]);
    }
    ret[0][0] = 'c';    
    // DPU_ASSERT(dpu_free(set));
}


void dpu_test(char *input, char* key, char* ret) {

	struct dpu_set_t set, dpu;
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
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
    DPU_ASSERT(dpu_free(set));
#endif
}
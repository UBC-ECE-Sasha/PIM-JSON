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
#if 1
void multi_dpu_test_org(char *input, long length, uint8_t** ret, uint32_t *records_len){
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
    int active_dpu = 0;
    printf("total record length is %ld\n", length);

    
    DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, (unsigned char*)"aabaa\n", MAX_KEY_SIZE));
    DPU_FOREACH (set, dpu) {
        if(offset + BUFFER_SIZE < length) {
    
            //DPU_ASSERT(dpu_copy_to(dpu, "input_length", 0, &input_length, sizeof(uint32_t)));
            status = dpu_copy_to_dpu(dpu, XSTR(DPU_BUFFER), 0, (unsigned char*)input+offset, BUFFER_SIZE);
           
            printf("dpu %d copy memory at offset %ld status %d\n", dpu_id, offset, status);
            offset += BUFFER_SIZE-(MAX_RECORD_SIZE/2);
            record_end = (char *)memchr(input+offset, '\n',
            length- offset);
            offset += record_end-(input+offset) +1;
            dpu_id++;

            printRecord(input+offset, (MAX_RECORD_SIZE/2));
            active_dpu++;
        }
        else if (offset < length) {
            status = dpu_copy_to_dpu(dpu, XSTR(DPU_BUFFER), 0, (unsigned char*)input+offset, ALIGN((length-offset), 8));
            DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, (unsigned char*)"aabaa\n", MAX_KEY_SIZE));     
            active_dpu++;
            printf("dpu copy finished\n");
            offset = length+1;       
        }
        else {
            printf("offset overflowed\n");
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
        if( i < active_dpu) {
            DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_LENGTH), 0, (uint8_t*)&(records_len[i]), sizeof(uint32_t)));
            if(records_len[i] != 0){
                DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_BUFFER), 0, (uint8_t*)(ret[i]), RETURN_RECORDS_SIZE));
            }
            i++;
        }
    }

    for(int j=0; j< active_dpu; j++) {
        printf("DPU %d\n found record length %d\n", j, records_len[j]);
    }
    // ret[0][0] = 'c';    
    // DPU_ASSERT(dpu_free(set));
}
#endif



char* get_curr_start(char* start, char* end) {
    long offset = BUFFER_SIZE-(MAX_RECORD_SIZE/2);
    char * curr = memchr(start+offset, '\n', end-start);
    if(curr != NULL) {
        return curr+1;
    }
    return NULL;
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

    //long offset = 0;
    unsigned int dpu_id = 0;
    char* curr_start = input;
    char* input_end = input+length;
    //dpu_error_t status;
    uint32_t input_length = 0;
    printf("total record length is %ld\n", length);
    uint32_t dpu_mram_buffer_start = 1024 * 1024;
    uint32_t dpu_mram_ret_buffer_start = ALIGN(dpu_mram_buffer_start + BUFFER_SIZE + 64, 64);

    // copy the key in for all DPUs - hardcoded now
    clock_t start, end;
    long copied_length =0;
    double duration = 0.0;
start = clock();
    DPU_FOREACH (set, dpu) {
    DPU_ASSERT(dpu_copy_to(dpu, XSTR(KEY), 0, (unsigned char*)"aaba\n", MAX_KEY_SIZE));
    DPU_ASSERT(dpu_copy_to(dpu, XSTR(DPU_BUFFER), 0, &dpu_mram_buffer_start, sizeof(uint32_t)));
    DPU_ASSERT(dpu_copy_to(dpu, XSTR(RECORDS_BUFFER), 0, &dpu_mram_ret_buffer_start, sizeof(uint32_t)));

        if(curr_start < input_end) {
            if(curr_start+ BUFFER_SIZE < input_end) {
                input_length = BUFFER_SIZE;
                DPU_ASSERT(dpu_copy_to(dpu, "input_length", 0, &input_length, sizeof(uint32_t)));

                DPU_ASSERT(dpu_copy_to_mram(dpu.dpu, dpu_mram_buffer_start, (unsigned char*)curr_start, BUFFER_SIZE, DPU_PRIMARY_MRAM));

                
                // if(dpu_copy_to_dpu(dpu, XSTR(DPU_BUFFER), 0, (unsigned char*)curr_start, BUFFER_SIZE) != 0){
                //     printf("dpu id %d copy memory failed at %ld\n",dpu_id, curr_start-input);
                // }

                curr_start = get_curr_start(curr_start, input_end);
                if(curr_start == NULL) {
                    printf("dpu id %d failed\n",dpu_id);
                }
                printf("host %d took %g s for coping %d total copied %ld\n", dpu_id, duration, BUFFER_SIZE, copied_length);
                copied_length +=BUFFER_SIZE;
            }
            else {
#if 1
                input_length = BUFFER_SIZE;
                char* src = (char*) malloc(BUFFER_SIZE);
                if(!memcpy(src, curr_start, (input_end-curr_start))) {
                     printf("memory copy failed\n");
                }
                if(dpu_copy_to_mram(dpu.dpu, dpu_mram_buffer_start, (unsigned char*)src, BUFFER_SIZE, DPU_PRIMARY_MRAM)){
                    printf("dpu id %d copy memory failed at %ld\n",dpu_id, curr_start-input);
                }
                else {
                    DPU_ASSERT(dpu_copy_to(dpu, "input_length", 0, &input_length, sizeof(uint32_t)));
                }
                printf("dpu copy finished\n");
                curr_start = input_end+1;
                copied_length +=(input_end-curr_start);
                
#endif
            }
        }
        dpu_id++;
    }
    end = clock();
    duration = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("host %d took %g s for coping %d total copied %ld\n", dpu_id, duration, BUFFER_SIZE, copied_length);

//	int err = dpu_launch(set, DPU_SYNCHRONOUS);
    start = clock();
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    end = clock();
    printf("dpu launch took %g s\n",((double) (end - start)) / CLOCKS_PER_SEC);
	// if (err != 0)
	// {
	// 	DPU_ASSERT(dpu_free(set));
    //     printf("dpu launch failed\n");
	// 	return;
	// }
#if 0
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
    int i =0;
    start = clock();
    DPU_FOREACH (set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, "output_length", 0, (uint8_t*)&(records_len[i]), sizeof(uint32_t)));
        if(records_len[i] != 0){
            //DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_BUFFER), 0, (uint8_t*)(ret[i]), RETURN_RECORDS_SIZE));
            DPU_ASSERT(dpu_copy_from_mram(dpu.dpu, (uint8_t*)(ret[i]), dpu_mram_ret_buffer_start, ALIGN(records_len[i]+32, 8), DPU_PRIMARY_MRAM));
        }
        i++;
    }
    end = clock();
    printf("dpu copy back took %g s\n",((double) (end - start)) / CLOCKS_PER_SEC);
#if DEBUG
    for(int j=0; j< NR_DPUS; j++) {
        printf("DPU %d\n found record length %d\n", j, records_len[j]);
    }
#endif
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
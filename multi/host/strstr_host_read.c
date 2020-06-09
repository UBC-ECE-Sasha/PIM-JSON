#include <dpu.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <dpu_log.h>
#include <time.h>
#include <string.h>


#include "common.h"
#ifndef DPU_BINARY
#define DPU_BINARY "build/strstr_dpu"
#endif
#include "dpu_host.h"
#define DPU_LOG_ENABLE

#define WRTIE_OUT 1

int main(int argc, char const *argv[])
{
    if(argc == 0) {
        printf("filename required\n");
    }
    
    // struct dpu_set_t set, dpu;
    // DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
    // DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    char *f;
    long size;
    struct stat s;
    const char * file_name = argv[1];
    // const char * key = argv[2]; 
    int fd = open (file_name, O_RDONLY);
    //uint8_t key_buffer[MAX_KEY_SIZE];

    /* Get the size of the file. */
    int status = fstat (fd, &s);
    size = s.st_size;
    struct timeval start;
	struct timeval end;

    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    printf("file size is %ld status %d buffer size %d\n", size, status, BUFFER_SIZE);
#if 0
    start = clock(); 
    host_strstr(f);
    end = clock();
    printf("host  took %g s\n", ((double) (end - start)) / CLOCKS_PER_SEC);
#endif
    uint8_t *ret_bufs[NR_DPUS];
	uint32_t records_len[NR_DPUS];
	for (int i=0; i<NR_DPUS; i++) {
	 	ret_bufs[i] = (uint8_t *) malloc(RETURN_RECORDS_SIZE);
	}

    gettimeofday(&start, NULL);
    multi_dpu_test(f, size, ret_bufs, records_len);
	gettimeofday(&end, NULL);
    
    double start_time = start.tv_sec + start.tv_usec / 1000000.0;
	double end_time = end.tv_sec + end.tv_usec / 1000000.0;
	printf("Host completed in %f seconds\n", end_time - start_time);

	for(int d=0; d< NR_DPUS; d++) {
		if(records_len[d] !=0) {
		for (uint32_t k=0; k< records_len[d]; k++){
			char c;
			c= ret_bufs[d][k];
			putchar(c);
		}
		printf("\n");
		}
	}
}
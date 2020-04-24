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
#include <dpu_log.h>
#include <time.h>
#include <string.h>

#include "common.h"
#ifndef DPU_BINARY
#define DPU_BINARY "build/strstr_dpu"
#endif
#define DPU_LOG_ENABLE

int main(int argc, char const *argv[])
{
    if(argc == 0) {
        printf("filename required\n");
    }
    
    struct dpu_set_t set, dpu;
    DPU_ASSERT(dpu_alloc(1, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    unsigned char *f;
    int size;
    struct stat s;
    const char * file_name = argv[1];
    // const char * key = argv[2]; 
    int fd = open (file_name, O_RDONLY);
    clock_t start, end;
    uint8_t key_buffer[MAX_KEY_SIZE];
    int byte_index;

    /* Get the size of the file. */
    int status = fstat (fd, &s);
    size = s.st_size;

    f = (unsigned char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    printf("file size is %d status %d buffer size %d\n", size, status, BUFFER_SIZE);


    // populate to dpu mram
    DPU_ASSERT(dpu_copy_to(set, XSTR(DPU_BUFFER), 0, f, BUFFER_SIZE));

    char key[] ="aabaa\n";
    int len = strlen(key);
    for (byte_index = 0; byte_index < len; byte_index++) {
        key_buffer[byte_index] = (uint8_t)key[byte_index];
    }

    DPU_ASSERT(dpu_copy_to(set, XSTR(KEY), 0, key_buffer, MAX_KEY_SIZE));



    start = clock();
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    end = clock();
    printf("dpu launch took %g s\n", ((double) (end - start)) / CLOCKS_PER_SEC);

#ifdef DPU_LOG_ENABLE 
    {
        unsigned int each_dpu = 0;
        printf("Display DPU Logs\n");
        DPU_FOREACH (set, dpu) {
        printf("DPU#%d:\n", each_dpu);
        //DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
        each_dpu++;
        }
    }
#endif
    uint32_t records_len=0;
    DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_LENGTH), 0, (uint8_t*)&records_len, sizeof(records_len)));
    printf("records length is %d\n", records_len);
    uint8_t ret[RETURN_RECORDS_SIZE]; 
    DPU_ASSERT(dpu_copy_from(dpu, XSTR(RECORDS_BUFFER), 0, (uint8_t*)&ret, RETURN_RECORDS_SIZE));

    // printf("length of returned records %ld\n", strlen((char*)ret));
    for (int k=0; k< RETURN_RECORDS_SIZE; k++){
        char c;
        c= ret[k];
        putchar(c);
    }
    printf("\n");
    exit(0);
    return 0;
}
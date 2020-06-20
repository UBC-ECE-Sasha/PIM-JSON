#ifndef __DPU_COMMON_H__
#define __DPU_COMMON_H__

#define XSTR(x) STR(x)
#define STR(x) #x

// /* DPU variable that will be read of write by the host */
#define DPU_BUFFER dpu_mram_buffer
#define DPU_CACHES dpu_wram_caches
#define DPU_RESULTS dpu_wram_results
#define KEY dpu_wram_key
#define RECORDS_BUFFER dpu_mram_ret_buffer
#define RECORDS_LENGTH dpu_records_len

#define BUFFER_SIZE (1 << 16)//(16<<20)//(3 << 19)
#define MAX_KEY_SIZE 32
#define RETURN_RECORDS_SIZE (1<<20)
#define MAX_RECORD_SIZE 2048

/* If you have a value that needs alignment to the nearest _width. For example,
0xF283 needs aligning to the next largest multiple of 16: 
ALIGN(0xF283, 16) will return 0xF290 */
#define ALIGN(_p, _width) (((unsigned int)_p + (_width-1)) & (0-_width))

#endif /* __DPU_COMMON_H__ */
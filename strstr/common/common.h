#ifndef __COMMON_H__
#define __COMMON_H__

#define XSTR(x) STR(x)
#define STR(x) #x

// /* DPU variable that will be read of write by the host */
#define DPU_BUFFER dpu_mram_buffer
#define DPU_CACHES dpu_wram_caches
#define DPU_RESULTS dpu_wram_results
#define KEY dpu_wram_key

// // /* Size of the buffer on which the strstr will be performed */
//#define BUFFER_SIZE (8 << 20)

// #define BUFFER_SIZE (4 << 20)
//#define BUFFER_SIZE (4 << 13)
//#define BUFFER_SIZE (2 << 16)
#define BUFFER_SIZE (1 << 20)
#define MAX_KEY_SIZE 32

/* Structure used by both the host and the dpu to communicate information */
typedef struct {
    uint32_t found;
    uint32_t cycles;
} dpu_results_t;

// #ifndef NR_TASKLETS
// #define NR_TASKLETS 24
// #endif

#endif /* __COMMON_H__ */

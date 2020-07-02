#ifndef __DPU_COMMON_H__
#define __DPU_COMMON_H__

#define XSTR(x) STR(x)
#define STR(x) #x
#include "../../PIM-common/common/include/common.h"
// /* DPU variable that will be read of write by the host */
#define DPU_BUFFER dpu_mram_buffer
#define DPU_CACHES dpu_wram_caches
#define DPU_RESULTS dpu_wram_results
#define KEY dpu_wram_key
#define RECORDS_BUFFER dpu_mram_ret_buffer
#define RECORDS_LENGTH dpu_records_len

#define BUFFER_SIZE (1 << 20)//(16<<20)//(3 << 19)
#define MAX_KEY_SIZE 8
#define RETURN_RECORDS_SIZE (8<<20)
#define MAX_RECORD_SIZE 2048

#endif /* __DPU_COMMON_H__ */
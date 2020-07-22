#ifndef DPU_HOST_H
#define DPU_HOST_H
#include "dpu_common.h"
void multi_dpu_test(char *input, unsigned int * keys, uint32_t keys_length, long length, uint32_t record_offsets[NR_DPUS][MAX_NUM_RETURNS], uint64_t input_offset[NR_DPUS][NR_TASKLETS], uint32_t output_count[NR_DPUS]);
#endif
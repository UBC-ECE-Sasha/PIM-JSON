#ifndef DPU_HOST_H
#define DPU_HOST_H
#include "dpu_common.h"
void multi_dpu_test(char *input, unsigned int * keys, int keys_length, long length, uint32_t record_offsets[NR_DPUS][NR_TASKLETS][MAX_NUM_RETURNS], uint32_t input_offset[NR_DPUS][NR_TASKLETS]);
#endif
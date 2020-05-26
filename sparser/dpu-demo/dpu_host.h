#ifndef DPU_HOST_H
#define DPU_HOST_H
void dpu_test(char *input, char* key, char* ret);
void multi_dpu_test(char *input, long length, uint8_t** ret);
#endif
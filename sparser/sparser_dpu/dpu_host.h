#ifndef DPU_HOST_H
#define DPU_HOST_H
void dpu_test(char *input, char* key, char* ret);
void multi_dpu_test(char *input, unsigned int * keys, int keys_length, long length, uint8_t** ret, uint32_t *records_len);
// void multi_dpu_test_org(char *input, unsigned int * keys, long length, uint8_t** ret, uint32_t *records_len);
#endif
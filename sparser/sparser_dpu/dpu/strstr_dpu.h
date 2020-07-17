#ifndef STRSTR_DPU_H
#define STRSTR_DPU_H

typedef struct in_buffer_context
{
	uint8_t *ptr;
	seqreader_buffer_t cache;
	seqreader_t sr;
    __mram_ptr uint8_t *mram_org;
    uint32_t curr;
	uint32_t length;
} in_buffer_context;

// Return values
typedef enum {
	STRSTR_JSON_ok = 0, // Success code
	STRSTR_JSON_INVALID_INPUT,		// Input file has an invalid format
} strstr_status;

typedef enum {
	record_start = 0, // Success code
	record_end,		// Input file has an invalid format
} record_state;


typedef struct record_descrip
{
    uint32_t length;
    __mram_ptr uint8_t * record_start;
    record_state state;
    uint32_t org;
	uint32_t str_mask;
} record_descrip;


#endif /* STRSTR_DPU_H */

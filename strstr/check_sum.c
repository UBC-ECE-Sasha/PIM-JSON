#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <stdbool.h>
#include <stdint.h>
#include <mutex.h>
#include <stdlib.h>
#include <stdio.h>
#include <perfcounter.h>

__mram_noinit long buffer_size_input;
__mram_noinit uint32_t checksum_output;

#include <devprivate.h>
int master() {
  unsigned int buffer_offset, buffer_size, checksum = 0;
  __dma_aligned uint8_t local_cache[256];

  buffer_size = buffer_size_input;
  buffer_offset = DPU_MRAM_HEAP_POINTER;

  tell(buffer_offset, "0");
  tell(buffer_size, "1");

  for (unsigned int bytes_read = 0; bytes_read < buffer_size;) {
    mram_read256(buffer_offset + bytes_read, local_cache);

    for (unsigned int byte_index = 0; (byte_index < 256) && (bytes_read < buffer_size); byte_index++, bytes_read++) {
      checksum += (uint32_t)local_cache[byte_index];
    }
  }


  checksum_output = checksum;
  return checksum;
}

int slave() {
  return 0;
}


int main() {
  int tasklet_id = me();
  tasklet_id == 0 ? master(): slave();
  printf("the tasklet id is %d",tasklet_id);

  return 0;
  // return tasklet_id == 0 ? master(): slave();
}
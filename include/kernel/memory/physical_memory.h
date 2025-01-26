#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define BLOCKS 1024 * 1024

void *allocate_blocks(uint32_t blocks);
void free_blocks(void *address, uint32_t blocks);

#endif // PHYSICAL_MEMORY_H

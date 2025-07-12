#ifndef PAGE_MEMORY_ALLOC_H
#define PAGE_MEMORY_ALLOC_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define SET_FRAME(entry, address) (*entry = (*entry & ~0xFFFFF000) | (address - 0xC0000000))

void page_init();
void *page_alloc(uint8_t zeroit);
void page_free(void *address);

#endif // PAGE_MEMORY_ALLOC_H

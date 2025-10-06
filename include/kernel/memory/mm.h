#ifndef PAGE_MEMORY_ALLOC_H
#define PAGE_MEMORY_ALLOC_H

#include "kernel/types.h"

#define PAGE_SIZE 4096

void pmm_init();
void *pmm_alloc_page();
void *pmm_alloc_pages(uint32_t npages);
void pmm_alloc_specific_page(void *phys);
void pmm_alloc_specific_pages(void *phys, uint32_t npages);
void pmm_free_page(void *phys);
void pmm_free_pages(void *phys, uint32_t npages);

#endif // PAGE_MEMORY_ALLOC_H

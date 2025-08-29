#include "kernel/memory/mm.h"
#include "kernel/memory/layout.h"

#include "kernel/panic.h"
#include "types/string.h"
#include "types/bitmap.h"
#include <stdint.h>

bitmap_t *page_bitmap = 0;

void pmm_init() {
	page_bitmap = create_bitmap(PAGE_COUNT);
	for (uint32_t i = 0; i < 128; i++) {
		page_bitmap->memory_map[i] = 0xFFFFFFFF;
		page_bitmap->memory_map[i + 256] = 0xFFFFFFFF;
	}
	page_bitmap->free_blocks -= 128 * 64;
}

void *pmm_alloc_page(void) {
	uint32_t page = allocate_block(page_bitmap);
	uint32_t address = page * PAGE_SIZE;
	return (void *)address;
}

void *pmm_alloc_pages(uint32_t npages) {
	uint32_t page = allocate_blocks(page_bitmap, npages);
	if (page)
		return NULL;
	uint32_t address = page * PAGE_SIZE;
	return (void *)address;
}

void pmm_alloc_specific_page(void *phys) {
	set_block(page_bitmap, (uint32_t)phys / PAGE_SIZE, 1);
}

void pmm_alloc_specific_pages(void *phys, uint32_t npages) {
	set_blocks(page_bitmap, (uint32_t)phys / PAGE_SIZE, npages, 1);
}

void pmm_free_page(void *phys) {
	set_block(page_bitmap, (uint32_t)phys / PAGE_SIZE, 0);
}

void pmm_free_pages(void *phys, uint32_t npages) {
	set_blocks(page_bitmap, (uint32_t)phys / PAGE_SIZE, npages, 0);
}
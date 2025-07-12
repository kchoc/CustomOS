#include "kernel/memory/page_memory_alloc.h"
#include "kernel/memory/layout.h"
#include "kernel/terminal.h"

#include "types/string.h"
#include "types/bitmap.h"

bitmap_t *page_bitmap = 0;

void page_init() {
	page_bitmap = create_bitmap(PAGE_COUNT);
	for (uint32_t i = 0; i < 128; i++) {
		page_bitmap->memory_map[i] = 0xFFFFFFFF;
		page_bitmap->memory_map[i + 256] = 0xFFFFFFFF;
	}
	page_bitmap->free_blocks -= 128 * 64;
}

void *page_alloc(uint8_t zeroit) {
	if (!page_bitmap) {
		printf("Page bitmap not initialized\n");
		return 0;
	}

	uint32_t page = allocate_block(page_bitmap);

	uint32_t address = page * PAGE_SIZE;

	if (zeroit) {
		memset((void *)address, 0, PAGE_SIZE);
	}

	return (void *)address;
}

void page_free(void *address) {
	if (!page_bitmap) {
		printf("Page bitmap not initialized\n");
		return;
	}

	uint32_t page = (uint32_t)address / PAGE_SIZE;

	free_block(page_bitmap, page);
}
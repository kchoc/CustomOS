#include "vm_phys.h"
#include "vm_page.h"

#include <vm/layout.h>

#include <kern/terminal.h>
#include <libkern/bitmap.h>
#include <kern/errno.h>
#include <kern/panic.h>

bitmap_t *page_bitmap = NULL;

size_t total_memory = 0;
size_t total_free_memory = 0;

int vm_phys_init(memory_map_entry_t* mem_map, size_t mem_map_length) {
	for (size_t i = 0; i < mem_map_length; i++) {
		if (mem_map[i].type == MEM_MAP_TYPE_AVAILABLE) {
			total_free_memory += mem_map[i].length;
		}
		total_memory += mem_map[i].length;
	}

	if (total_memory == 0) return ENOMEM;

	// Create bitmap for physical memory management
	size_t page_count = total_memory / PAGE_SIZE;
	page_bitmap = create_bitmap(page_count);

	// Mark reserved pages as allocated
	for (size_t i = 0; i < mem_map_length; i++) {
		if (mem_map[i].type != MEM_MAP_TYPE_AVAILABLE) {
			size_t start_page = mem_map[i].base_addr / PAGE_SIZE;
			size_t num_pages = (mem_map[i].length + PAGE_SIZE - 1) / PAGE_SIZE;
			set_blocks(page_bitmap, start_page, num_pages, 1);
		}
	}

	// Mark pages used by the kernel as allocated (for simplicity, we assume the kernel occupies the first 16MB)
	size_t kernel_pages = (16 * 1024 * 1024) / PAGE_SIZE;
	set_blocks(page_bitmap, 0, kernel_pages, 1);

	return 0;
}

void vm_phys_dump_info() {
	printf("Total Memory: %u MB\n", total_memory / (1024 * 1024));
	printf("Free Memory: %u MB\n", total_free_memory / (1024 * 1024));
}

paddr_t vm_phys_alloc_page() {
    paddr_t page = allocate_block(page_bitmap);
    paddr_t address = page * PAGE_SIZE;
    return address;
}

paddr_t vm_phys_alloc_pages(size_t npages) {
    paddr_t page = allocate_blocks(page_bitmap, npages);
    if (page) return ENOMEM;
    paddr_t address = page * PAGE_SIZE;
    return address;
}

void vm_phys_alloc_specific_page(paddr_t phys) {
    set_block(page_bitmap, phys / PAGE_SIZE, 1);
}

void vm_phys_alloc_specific_pages(paddr_t phys, uint32_t npages) {
    set_blocks(page_bitmap, phys / PAGE_SIZE, npages, 1);
}

void vm_phys_free_page(paddr_t phys) {
    set_block(page_bitmap, phys / PAGE_SIZE, 0);
}

void vm_phys_free_pages(paddr_t phys, uint32_t npages) {
    set_blocks(page_bitmap, phys / PAGE_SIZE, npages, 0);
}

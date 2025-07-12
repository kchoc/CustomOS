#include "kernel/memory/page_memory_alloc.h"
#include "kernel/memory/page.h"
#include "kernel/memory/layout.h"
#include "kernel/terminal.h"
#include "types/string.h"
#include <stdint.h>
#include <stdio.h>

page_table_t* current_pd = (page_table_t *)PAGE_DIRECTORY_ADDRESS;

page_table_t *page_table_create() {
    return page_alloc(0);
}

int page_table_get_map(uint32_t virt, uint32_t *phys, uint32_t *flags) {
	page_table_t *table;
	page_entry_t *entry;

	uint32_t table_idx = virt >> 22;
	uint32_t entry_idx = virt >> 12 & 0x3FF;

	// Check if the table is mapped
	table = (page_table_t *)PAGE_DIRECTORY_ADDRESS;
	entry = &table->entries[table_idx];
	if (!(*entry & PAGE_FLAG_PRESENT))
		return -1;

	// Check if the frame is mapped
	table = (page_table_t *)(PAGE_TABLES_ADDRESS + (table_idx << 12));
	entry = &table->entries[entry_idx];
	if (!(*entry & PAGE_FLAG_PRESENT))
		return -1;

	*phys = *entry & PAGE_DIRECTORY_ADDRESS;

	if (flags)
		*flags = *entry & 0x00000FFF; // Extract flags

	return 0;
}

int page_table_map(uint32_t virt, uint32_t *phys, uint32_t flags) {
	page_table_t *table;
	page_entry_t *entry;

	uint32_t table_idx = virt >> 22;
	uint32_t entry_idx = (virt >> 12) & 0x3FF;

	printf("Let find the issue\n");

	// Use recursive mapping to get the page table virtual address
	// Explanation: The last page directory entry maps the page directory as a page table,
	// then this table maps all the page tables as page frames including the page directory
	// The page directory can then be accessed at the virtual address 0xFFFFF000
	table = current_pd;
	entry = &table->entries[table_idx];

	if (!(*entry & PAGE_FLAG_PRESENT)) {
		table = page_table_create();
		if (!table)
			return -1;

		*entry = (page_entry_t)table | PAGE_FLAG_PRESENT | PAGE_FLAG_READWRITE | PAGE_FLAG_USER;
		table = (page_table_t *)(PAGE_TABLES_ADDRESS + (table_idx << 12));
		memset(table, 0, PAGE_SIZE);
	} else {
		table = (page_table_t *)(PAGE_TABLES_ADDRESS + (table_idx << 12));
	}

	entry = &table->entries[entry_idx];
	if (*entry & PAGE_FLAG_PRESENT)
		return -1;

	if (flags & PAGE_FLAG_ALLOCATE) {
		*phys = (uint32_t)page_alloc(flags & PAGE_FLAG_CLEAR);
		if (!*phys)
			return -1;
	}

	*entry = *phys | PAGE_FLAG_PRESENT | PAGE_FLAG_USER | PAGE_FLAG_READWRITE;

	return 0;
}

int page_table_unmap(uint32_t virt_addr) {
	page_table_t *table;
	page_entry_t *entry;

	uint32_t table_idx = virt_addr >> 22;
	uint32_t entry_idx = (virt_addr >> 12) & 0x3FF;

	// Use recursive mapping to get the page table virtual address
	table = (page_table_t *)PAGE_DIRECTORY_ADDRESS;
	entry = &table->entries[table_idx];

	if (!(*entry & PAGE_FLAG_PRESENT))
		return -1;

	table = (page_table_t *)(PAGE_TABLES_ADDRESS + (table_idx << 12));
	entry = &table->entries[entry_idx];

	if (!(*entry & PAGE_FLAG_PRESENT))
		return -1;

	*entry = 0; // Unmap the entry

	return 0;
}

void page_table_alloc(page_table_t *pd, uint32_t virt, uint32_t length, uint32_t flags) {
	uint32_t npages = length / PAGE_SIZE;

	if (length % PAGE_SIZE)
		npages++;
	
	virt &= 0xFFFFF000;


	while (npages) {
		uint32_t phys;
		if (!page_table_get_map(virt, &phys, 0))
			page_table_map(virt, 0, flags | PAGE_FLAG_ALLOCATE);
		
		virt += PAGE_SIZE;
		npages--;
	}

	page_table_refresh();
}

void page_table_free(page_table_t *pd, uint32_t virt, uint32_t length) {
	uint32_t npages = length / PAGE_SIZE;

	if (length % PAGE_SIZE)
		npages++;
	
	virt &= 0xFFFFF000;

	while (npages) {
		uint32_t phys;
		uint32_t flags;
		if (page_table_get_map(virt, &phys, &flags)) {
			page_table_unmap(virt);
			// If the page is allocated, free it
			if (flags & PAGE_FLAG_ALLOCATE)
				page_free((void *)phys);
		}

		virt += PAGE_SIZE;
		npages--;
	}
}

void page_table_destroy(page_table_t *pd) {
	page_table_t *table;
	page_entry_t *entry;

	for (int i = 0; i < 768; i++) {
		entry = &pd->entries[i];
		if (!(*entry & PAGE_FLAG_PRESENT))
			continue;

		table = (page_table_t *)(*entry & 0xFFFFF000);
		for (int j = 0; j < 1024; j++) {
			if (table->entries[j] & PAGE_FLAG_PRESENT) {
				page_free((void *)(table->entries[j] & 0xFFFFF000));
			}
		}

		page_free(table);
	}
}

page_table_t *new_task_page_table() {
    // Allocate a new page directory (unmapped initially)
    page_table_t *new_pd_phys = page_table_create();
    if (!new_pd_phys)
        return NULL;

    // Unmap the current working page table from the current page directory
    page_table_unmap((uint32_t)PAGE_TABLE_EDIT_ADDRESS);

    // Map the new page directory into the current working page table
     if (page_table_map(PAGE_TABLE_EDIT_ADDRESS, (uint32_t *)&new_pd_phys, PAGE_FLAG_PRESENT | PAGE_FLAG_READWRITE))
     	return NULL;

    // Copy the kernel-space entries only (typically the upper 256 entries)
    page_table_t *new_pd = (page_table_t *)PAGE_TABLE_EDIT_ADDRESS;
    for (int i = 0; i < 255; i++)
        new_pd->entries[i + 768] = current_pd->entries[i + 768];

    // Set up recursive mapping in new PD
    new_pd->entries[1023] = (page_entry_t)new_pd_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_READWRITE;

    return new_pd_phys;
}

// Switch to a new page table and return the old one
page_table_t *page_table_load(page_table_t *pd) {
	page_table_t *old_pd = 0;
	asm volatile("mov %%cr3, %0" : "=r"(old_pd));
	asm volatile("mov %0, %%cr3" : : "r"(pd));
	return old_pd;
}

void page_table_refresh() {
	asm volatile("mov %cr3, %eax");
	asm volatile("mov %eax, %cr3");
}

void page_table_enable() {
	asm volatile("mov %cr0, %eax");
	asm volatile("or $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");
}

page_table_t *get_current_page_table_phys() {
	page_table_t *pd;
	asm volatile("mov %%cr3, %0" : "=r"(pd));
	return pd;
}

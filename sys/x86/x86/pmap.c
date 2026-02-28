#include <machine/pmap.h>
#include <machine/page_table.h>

#include <kern/errno.h>
#include <vm/layout.h>
#include <vm/kmalloc.h>
#include <vm/vm_phys.h>
#include "string.h"
#include "vm/types.h"

void tlb_invlpg(void* addr) {
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

void tlb_flush() {
	asm volatile("mov %cr3, %eax");
	asm volatile("mov %eax, %cr3");
}

void switch_page_directory(page_table_t** pd_ptr) {
	if (*pd_ptr == *current_pd_addr || *pd_ptr == NULL) return;
	asm volatile("mov %0, %%cr3" : : "r"(*pd_ptr) : "memory");
}

int pmap_init() {
	for (uint32_t i = 0; i < KERNEL_PAGE_ENTRIES - 1; i++) {
		if (!(current_pd->entries[i + KERNEL_PAGE_ENTRY_START] & 0x1)) {
			page_table_t *table = (page_table_t*)vm_phys_alloc_page();
			if (is_errno((paddr_t)table)) return -ENOMEM;

			current_pd->entries[i + KERNEL_PAGE_ENTRY_START] = (page_entry_t)table | VM_PROT_READ | VM_PROT_WRITE | VM_PROT_GLOBAL;
			tlb_invlpg(&current_pts[i + KERNEL_PAGE_ENTRY_START]);
			memset(&current_pts[i + KERNEL_PAGE_ENTRY_START], 0, PAGE_SIZE);
		}
	}
	return 0;
}

pmap_t* pmap_create() {
	pmap_t* pmap = kmalloc(sizeof(pmap_t));
	if (!pmap) return NULL;

	pmap->pd = (page_table_t*)vm_phys_alloc_page();

	return pmap;
}

static void free_table(page_table_t* table, int level) {
	for (int i = 0; i < (level == 0 ? KERNEL_PAGE_ENTRY_START : KERNEL_PAGE_ENTRIES); i++) {
		if (table->entries[i] & VM_PROT_READ && !((table->entries[i]) & VM_PROT_HUGE)) {
			page_table_t* next_table = (page_table_t*)(table->entries[i] & 0xFFFFF000);
			if (level != PAGE_TABLE_LEVELS - 2) // Don't free page frames, they might be shared and should be freed by the vm system when the last reference is removed
				free_table(next_table, level + 1);
			vm_phys_free_page((paddr_t)next_table);
		}
	}
}

void pmap_destroy(pmap_t* pmap) {
	if (!pmap) return;

	// Free page tables but not the page frames since they might be shared and should be freed by the vm system when the last reference is removed
	// Im going to use recursion to free all page tables but this could be optimized by keeping track of allocated page tables in the pmap structure and freeing them iteratively instead
	free_table((page_table_t*)pmap->pd, 0);

	vm_phys_free_page((paddr_t)pmap->pd);
	kfree(pmap);
}

void pmap_activate(pmap_t *pmap) {
	asm volatile("mov %0, %%cr3" : : "r"(pmap->pd) : "memory");
}

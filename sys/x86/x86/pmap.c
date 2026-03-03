#include <machine/pmap.h>
#include <machine/page_table.h>
#include <vm/vm_space.h>

#include <kern/errno.h>
#include <vm/layout.h>
#include <vm/kmalloc.h>
#include <vm/vm_phys.h>
#include "string.h"
#include "vm/types.h"

#include <kern/panic.h>

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
	pmap_enter(kernel_vm_space->arch, PAGE_TABLE_EDIT_ADDRESS, (paddr_t)pmap->pd, VM_PROT_READ | VM_PROT_WRITE, PMAP_FLAG_NONE);
	for (int i = KERNEL_PAGE_ENTRIES; i < PAGE_ENTRIES_PER_TABLE; i++) {
		edit_pd->entries[i] = current_pd->entries[i];
	}
	pmap_remove(kernel_vm_space->arch, PAGE_TABLE_EDIT_ADDRESS, PAGE_TABLE_EDIT_ADDRESS + PAGE_SIZE);

	return pmap;
}

void pmap_activate(pmap_t *pmap) {
	asm volatile("mov %0, %%cr3" : : "r"(pmap->pd) : "memory");
}

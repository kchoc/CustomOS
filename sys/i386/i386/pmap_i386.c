#include <machine/pmap.h>
#include <machine/page_table.h>

#include <vm/vm_phys.h>
#include <kern/errno.h>

#include "string.h"
#include "vm/types.h"

#define TABLE_IDX(virt)      ((uint32_t)virt >> 22)
#define ENTRY_IDX(virt)      (((uint32_t)virt >> 12) & 0x3FF)

int pmap_enter(pmap_t *pmap, vaddr_t virt, paddr_t phys, vm_prot_t prot, vm_flags_t flags) {
	SWITCH_SPACE(pmap);

	uint32_t table_idx = TABLE_IDX(virt);
	uint32_t entry_idx = ENTRY_IDX(virt);

	page_table_t *table = current_pd;
	if (!(table->entries[table_idx] & 0x1)) {
		page_table_t *new_table = (page_table_t*)vm_phys_alloc_page();
		if (is_errno((paddr_t)new_table)) return ENOMEM;

		table->entries[table_idx] = (page_entry_t)new_table | VM_PROT_READ | VM_PROT_WRITE;
		tlb_invlpg(&current_pts[table_idx]);
		memset(&current_pts[table_idx], 0, PAGE_SIZE);
	}

	page_entry_t *entry = &current_pts[table_idx].entries[entry_idx];

	if (*entry & VM_PROT_READ) {
		return EEXIST; // Already mapped
	}

	*entry = (page_entry_t)(phys & 0xFFFFF000) | (prot & 0xFFF) | VM_PROT_READ;
	tlb_invlpg((void*)virt);

	if (flags & VM_MAP_ZERO) {
		memset((void*)virt, 0, PAGE_SIZE);
	}

	return 0;
}

void pmap_remove(pmap_t *pmap, vaddr_t sva, vaddr_t eva) {
	SWITCH_SPACE(pmap);

	for (vaddr_t addr = sva; addr < eva; addr += PAGE_SIZE) {
		uint32_t table_idx = TABLE_IDX(addr);
		uint32_t entry_idx = ENTRY_IDX(addr);

		page_table_t *table = current_pd;
		if (!(table->entries[table_idx] & 0x1)) {
			continue; // Page table not present
		}

		page_entry_t *entry = &current_pts[table_idx].entries[entry_idx];
		if (!(*entry & VM_PROT_READ)) {
			continue; // Page not mapped
		}

		*entry = 0; // Clear the entry
		tlb_invlpg((void*)addr);
	}
}

paddr_t pmap_extract(pmap_t *pmap, vaddr_t virt) {
	SWITCH_SPACE(pmap);

	uint32_t table_idx = TABLE_IDX(virt);
	uint32_t entry_idx = ENTRY_IDX(virt);

	page_table_t *table = current_pd;
	if (!(table->entries[table_idx] & 0x1)) {
		return -ENOENT; // Page table not present
	}

	page_entry_t *entry = &current_pts[table_idx].entries[entry_idx];
	if (!(*entry & VM_PROT_READ)) {
		return -ENOENT; // Page not mapped
	}

	return (paddr_t)(*entry & 0xFFFFF000);
}


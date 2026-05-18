#include <vm/kmalloc.h>
#include <vm/types.h>
#include <vm/vm_phys.h>

#include <machine/page_table.h>
#include <machine/pmap.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/spinlock.h>
#include <kern/terminal.h>

#include <string.h>

#define TABLE_IDX(virt) ((uint32_t)virt >> 22)
#define ENTRY_IDX(virt) (((uint32_t)virt >> 12) & 0x3FF)

void pmap_destroy(pmap_t* pmap)
{
    page_table_t* old_pd = *current_pd_addr;
    switch_page_directory(&pmap->pd); // Switch to the kernel page directory
    
    WITH_SPINLOCK(pmap->lock)

    // Assumes that all user-space mappings have been removed, so we only need to free the page
    // tables
    for (int i = 0; i < KERNEL_PAGE_ENTRY_START; i++) {
        if (current_pd->entries[i] & 0x1) {
            page_table_t* table = (page_table_t*)(current_pd->entries[i] & ~PAGE_MASK);
            for (int j = 0; j < PAGE_ENTRIES_PER_TABLE; j++) {
                if (current_pts[i].entries[j] & 0x1) {
                    // If the page is present, free the physical page it maps to
                    paddr_t phys = current_pts[i].entries[j] & ~PAGE_MASK;
                    vm_phys_free_page(phys);
                }
            }
            memset(&current_pts[i], 0, PAGE_SIZE);
            vm_phys_free_page((paddr_t)table);
        }
    }

    switch_page_directory(&old_pd); // Switch back to the original page directory

    END_WITH_SPINLOCK

    vm_phys_free_page((paddr_t)pmap->pd);
    kfree(pmap);
}

int pmap_enter(pmap_t* pmap, vaddr_t virt, paddr_t phys, vm_prot_t prot, pmap_flags_t flags)
{
    uint32_t table_idx = TABLE_IDX(virt);
    uint32_t entry_idx = ENTRY_IDX(virt);

    WITH_SPINLOCK(pmap->lock)

    page_table_t* table = current_pd;
    if (!(table->entries[table_idx] & 0x1)) {
        page_table_t* new_table = (page_table_t*)vm_phys_alloc_page();
        if (is_errno((paddr_t)new_table))
            return -ENOMEM;
        table->entries[table_idx] =
            (page_entry_t)new_table | VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER;
        tlb_invlpg(&current_pts[table_idx]);
        memset(&current_pts[table_idx], 0, PAGE_SIZE);
    }

    page_entry_t* entry = &current_pts[table_idx].entries[entry_idx];

    if (*entry & VM_PROT_READ) {
        return -EEXIST; // Already mapped
    }

    if (flags & PMAP_FLAG_NOCACHE) {
        prot |= VM_PROT_NOCACHE;
    }

    *entry = (page_entry_t)(phys & 0xFFFFF000) | (prot & 0xFFF) | VM_PROT_READ;
    tlb_invlpg((void*)virt);

    if (flags & PMAP_FLAG_ZERO) {
        memset((void*)virt, 0, PAGE_SIZE);
    }

    END_WITH_SPINLOCK

    return 0;
}

void pmap_remove(pmap_t* pmap, vaddr_t sva, vaddr_t eva)
{
    SWITCH_SPACE(pmap);

    WITH_SPINLOCK(pmap->lock)

    for (vaddr_t addr = sva; addr < eva; addr += PAGE_SIZE) {
        uint32_t table_idx = TABLE_IDX(addr);
        uint32_t entry_idx = ENTRY_IDX(addr);

        page_table_t* table = current_pd;
        if (!(table->entries[table_idx] & 0x1)) {
            continue; // Page table not present
        }

        page_entry_t* entry = &current_pts[table_idx].entries[entry_idx];
        if (!(*entry & VM_PROT_READ)) {
            continue; // Page not mapped
        }

        *entry = 0; // Clear the entry
        tlb_invlpg((void*)addr);
    }

    END_WITH_SPINLOCK
}

void pmap_protect(pmap_t* pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
    SWITCH_SPACE(pmap);

    WITH_SPINLOCK(pmap->lock)

    for (vaddr_t addr = sva; addr < eva; addr += PAGE_SIZE) {
        uint32_t table_idx = TABLE_IDX(addr);
        uint32_t entry_idx = ENTRY_IDX(addr);

        page_table_t* table = current_pd;
        if (!(table->entries[table_idx] & 0x1)) {
            continue; // Page table not present
        }

        page_entry_t* entry = &current_pts[table_idx].entries[entry_idx];
        if (!(*entry & VM_PROT_READ)) {
            continue; // Page not mapped
        }

        *entry = (page_entry_t)(((uintptr_t)*entry & ~PAGE_MASK) | (prot & PAGE_MASK));
        tlb_invlpg((void*)addr);
    }

    END_WITH_SPINLOCK
}

paddr_t pmap_extract(pmap_t* pmap, vaddr_t virt)
{
    SWITCH_SPACE(pmap);

    WITH_SPINLOCK(pmap->lock)

    uint32_t table_idx = TABLE_IDX(virt);
    uint32_t entry_idx = ENTRY_IDX(virt);

    page_table_t* table = current_pd;
    if (!(table->entries[table_idx] & 0x1)) {
        return -ENOENT; // Page table not present
    }

    page_entry_t* entry = &current_pts[table_idx].entries[entry_idx];
    if (!(*entry & VM_PROT_READ)) {
        return -ENOENT; // Page not mapped
    }

    return (paddr_t)(*entry & 0xFFFFF000);

    END_WITH_SPINLOCK
}

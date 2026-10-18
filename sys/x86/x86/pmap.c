#include <machine/page_table.h>
#include <machine/pmap.h>
#include <vm/vm_space.h>
#include <vm/vm_temp.h>

#include "vm/types.h"
#include <kern/errno.h>
#include <vm/kmalloc.h>
#include <vm/layout.h>
#include <vm/vm_phys.h>

#include <kern/panic.h>
#include <kern/terminal.h>

page_table_t** current_pd_addr = (page_table_t**)0xFFFFFFFC;
page_table_t*  current_pd      = (page_table_t*)PAGE_DIRECTORY_ADDRESS;
page_table_t*  current_pts     = (page_table_t*)PAGE_TABLES_ADDRESS;

pmap_t kernel_pmap_store = {
    .pd   = 0,
    .lock = SPINLOCK_INITIALIZER,
};

void tlb_invlpg(void* addr)
{
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

void tlb_flush()
{
    asm volatile("mov %cr3, %eax");
    asm volatile("mov %eax, %cr3");
}

void switch_page_directory(page_table_t** pd_ptr)
{
    if (PAGE_ALIGN_DOWN(*pd_ptr) == PAGE_ALIGN_DOWN(*current_pd_addr) || *pd_ptr == NULL)
        return;
    asm volatile("mov %0, %%cr3" : : "r"(*pd_ptr) : "memory");
}

void pmap_debug(pmap_t* pmap)
{
    printf("Page Directory at: %p\n", *current_pd_addr);

    VM_TEMP_GUARD page_table_t* edit_pd =
        vm_temp_map_page((paddr_t)pmap->pd, VM_PROT_READ | VM_PROT_WRITE);
    for (int i = 767; i < 768; i++) {
        if (edit_pd->entries[i] & 0x1) {
            printf("PD Entry %d: %p\n", i, edit_pd->entries[i] & 0xFFFFF000);
            i += 15;
        }
    }
}

int pmap_init()
{
    kernel_pmap->pd = *current_pd_addr;

    return 0;
}

pmap_t* pmap_create()
{
    pmap_t* pmap = kmalloc(sizeof(pmap_t));
    if (!pmap)
        return ERR_PTR(-ENOMEM);

    pmap->pd = (page_table_t*)vm_phys_alloc_page();
    if (is_errno((paddr_t)pmap->pd)) {
        kfree(pmap);
        return ERR_PTR(-ENOMEM);
    }
    pmap->lock = SPINLOCK_INITIALIZER;

    VM_TEMP_GUARD page_table_t* edit_pd =
        vm_temp_map_page((paddr_t)pmap->pd, VM_PROT_READ | VM_PROT_WRITE);

    // Copy kernel mappings from the current page directory, leaving user-space entries as not
    // present and setting the last entry to point to the new page directory itself for recursive
    // mapping.
    for (int i = KERNEL_PAGE_ENTRY_START; i < PAGE_ENTRIES_PER_TABLE - 1; i++) {
        edit_pd->entries[i] = current_pd->entries[i];
    }

    edit_pd->entries[PAGE_ENTRIES_PER_TABLE - 1] =
        ((uint32_t)pmap->pd) | VM_PROT_READ | VM_PROT_WRITE;

    return pmap;
}

void pmap_activate(pmap_t* pmap)
{
    if (pmap == NULL || pmap->pd == NULL)
        PANIC("pmap_activate: Invalid page directory");
    asm volatile("mov %0, %%cr3" : : "r"(pmap->pd) : "memory");
}

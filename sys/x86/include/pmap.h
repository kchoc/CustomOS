#ifndef X86_PMAP_H
#define X86_PMAP_H

#include "page_table.h"

#include <vm/layout.h>
#include <vm/types.h>

#include <kern/compiler.h>
#include <kern/spinlock.h>

#include <stddef.h>

// TODO: Add compiler check for i386 or amd64 for PTLs
#define PAGE_TABLE_LEVELS       2
#define KERNEL_PAGE_ENTRIES     256
#define PAGE_ENTRIES_PER_TABLE  1024
#define KERNEL_PAGE_ENTRY_START (PAGE_ENTRIES_PER_TABLE - KERNEL_PAGE_ENTRIES)

extern page_table_t** current_pd_addr;
extern page_table_t*  current_pd;
extern page_table_t*  current_pts;

void tlb_invlpg(void* addr);
void tlb_flush();
void switch_page_directory(page_table_t** pd_ptr);

#define SWITCH_SPACE(pmap)                                                                         \
    page_table_t* old_pd __cleanup(switch_page_directory) = *current_pd_addr;                      \
    if (pmap && old_pd != pmap->pd)                                                                \
        switch_page_directory((void*)&pmap->pd);

typedef struct pmap {
    page_table_t* pd; // Page directory
    spinlock_t    lock;
} pmap_t;

#endif // X86_PMAP_H

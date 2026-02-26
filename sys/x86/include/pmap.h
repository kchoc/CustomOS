#ifndef X86_PMAP_H
#define X86_PMAP_H

#include "page_table.h"

#include <vm/layout.h>
#include <vm/types.h>
#include <stddef.h>
#include <kern/compiler.h>

//TODO: Add compiler check for i386 or amd64 for PTLs
#define PAGE_TABLE_LEVELS 2
#define KERNEL_PAGE_ENTRIES 256
#define PAGE_ENTRIES_PER_TABLE 1024
#define KERNEL_PAGE_ENTRY_START (PAGE_ENTRIES_PER_TABLE - KERNEL_PAGE_ENTRIES)

static page_table_t** current_pd_addr = (page_table_t **)0xFFFFFFFC;
static page_table_t* current_pd = (page_table_t *)PAGE_DIRECTORY_ADDRESS;
static page_table_t* current_pts = (page_table_t *)PAGE_TABLES_ADDRESS;
static page_table_t* edit_pd = (page_table_t*)PAGE_TABLE_EDIT_ADDRESS;

void tlb_invlpg(void* addr);
void tlb_flush();
void switch_page_directory(page_table_t** pd_ptr);

#define SWITCH_SPACE(pmap) \
	page_table_t* old_pd __cleanup(switch_page_directory) = *current_pd_addr; \
	if (pmap && old_pd != pmap->pd) switch_page_directory((void*)&pmap->pd); \

typedef struct pmap {
	page_table_t* pd; // Page directory
} pmap_t;

pmap_t* pmap_create();
pmap_t* pmap_fork(pmap_t* parent);
void pmap_destroy(pmap_t* pmap);
int  pmap_enter(pmap_t* pmap, vaddr_t virt, paddr_t phys, vm_prot_t prot, vm_flags_t flags);
void pmap_remove(pmap_t* pmap, vaddr_t sva, vaddr_t eva);
void pmap_protect(pmap_t* pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot);
paddr_t pmap_extract(pmap_t* pmap, vaddr_t virt);
void pmap_activate(pmap_t* pmap);

#endif // X86_PMAP_H

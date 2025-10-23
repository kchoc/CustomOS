#include "kernel/memory/mm.h"
#include "kernel/memory/vm.h"
#include "kernel/memory/layout.h"
#include "kernel/memory/vmspace.h"
#include "kernel/terminal.h"
#include "kernel/panic.h"
#include "types/string.h"

#define TABLE_IDX(virt)      ((uint32_t)virt >> 22)
#define ENTRY_IDX(virt)      (((uint32_t)virt >> 12) & 0x3FF)
#define SIZE_TO_PAGES(size)  (((size) + PAGE_SIZE - 1) / PAGE_SIZE)

page_table_t* current_pd = (page_table_t *)PAGE_DIRECTORY_ADDRESS;
page_table_t* current_pts = (page_table_t *)PAGE_TABLES_ADDRESS;

page_entry_t kernel_pd_entries[255] = {0};

static inline void tlb_invlpg(void *addr) {
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static void tlb_flush() {
    asm volatile("mov %cr3, %eax");
    asm volatile("mov %eax, %cr3");
}

//TODO: There might be an issue with page unmapping if the page tables are not allocated yet

void vmm_init(void) {
    // Create page tables for the kernel space or use the existing ones
    for (uint32_t i = 0; i < 255; i++) {
        if (!(current_pd->entries[i + 768] & VM_PROT_PRESENT)) {
            page_table_t *table = pmm_alloc_page();
            if (!table)
                PANIC("vmm_init: Out of memory");

            current_pd->entries[i + 768] = (page_entry_t)table | VM_PROT_PRESENT | VM_PROT_READWRITE | VM_PROT_GLOBAL;
            memset(&current_pts[i + 768], 0, PAGE_SIZE);

        }
        kernel_pd_entries[i] = current_pd->entries[i + 768];
    }
}

int page_table_get_map(uint32_t virt, uint32_t *phys, uint32_t *flags) {
    page_table_t *table;
    page_entry_t *entry;

    uint32_t table_idx = virt >> 22;
    uint32_t entry_idx = virt >> 12 & 0x3FF;

    // Check if the table is mapped
    table = (page_table_t *)PAGE_DIRECTORY_ADDRESS;
    entry = &table->entries[table_idx];
    if (!(*entry & VM_PROT_PRESENT))
        
        return -1;

    // Check if the frame is mapped
    table = (page_table_t *)(PAGE_TABLES_ADDRESS + (table_idx << 12));
    entry = &table->entries[entry_idx];
    if (!(*entry & VM_PROT_PRESENT))
        return -1;

    *phys = *entry & PAGE_DIRECTORY_ADDRESS;

    if (flags)
        *flags = *entry & 0x00000FFF; // Extract flags

    return 0;
}

int alloc_table(uint32_t table_idx, int prot) {
    page_table_t *table;
    page_entry_t *entry;

    table = current_pd;
    entry = &table->entries[table_idx];

    if (!(*entry & VM_PROT_PRESENT)) {

        table = pmm_alloc_page();
        if (!table) return -1;

        *entry = (page_entry_t)table | (prot & 0xFFF) | VM_PROT_PRESENT;
        tlb_invlpg(&current_pts[table_idx]); // Not sure if this is needed here but just in case
        memset(&current_pts[table_idx], 0, PAGE_SIZE);
    }

    return 0;
}

void *vmm_map(void *virt, uintptr_t phys, size_t size, int prot, int flags) {
    size_t mapped = 0;

    page_table_t *table;
    page_entry_t *entry;

    void* cur_virt = virt;
    uintptr_t cur_phys = phys;

    uint32_t table_idx = TABLE_IDX(virt);
    uint32_t entry_idx = ENTRY_IDX(virt);

    table = current_pd;
    entry = &table->entries[table_idx];
    if (alloc_table(table_idx, prot)) return NULL;
    table = &current_pts[table_idx];

    while (mapped < size) {
        if (entry_idx > 1023) {
            entry_idx = 0;
            table_idx++;
            if (alloc_table(table_idx, prot)) return NULL;
            table = &current_pts[table_idx];
        }
        entry = &table->entries[entry_idx];
        if (*entry & VM_PROT_PRESENT) {
            printf("vmm_map: virtual address %x already mapped to phys %x\n", cur_virt, *entry & 0xFFFFF000);
            if (!(flags & VM_MAP_FORCE))
                return NULL;

            if (flags & VM_MAP_FREE)
                pmm_free_page((void *)(*entry & 0xFFFFF000));
        }

        if (flags & VM_MAP_PHYS) {
            *entry = (page_entry_t)(cur_phys & 0xFFFFF000) | (prot & 0xFFF) | VM_PROT_PRESENT;
            cur_phys += PAGE_SIZE;
        } else {
            cur_phys = (uint32_t)pmm_alloc_page();
            if (!cur_phys) return NULL;
            *entry = (page_entry_t)(cur_phys & 0xFFFFF000) | (prot & 0xFFF) | VM_PROT_PRESENT;
        }

        tlb_invlpg(cur_virt); // Not sure if this is needed here but just in case
        cur_virt += PAGE_SIZE;
        mapped += PAGE_SIZE;
        entry_idx++;
    }


    if (flags & VM_MAP_ZERO)
        memset(virt, 0, size);

    if (flags & VM_MAP_PHYS)
        pmm_alloc_specific_pages((void *)(phys & 0xFFFFF000), SIZE_TO_PAGES(size));

    return virt;
}

void vmm_unmap(void *virt, size_t size) {
    size_t unmapped = 0;

    page_table_t *table;
    page_entry_t *entry;

    uint32_t table_idx = TABLE_IDX(virt);
    uint32_t entry_idx = ENTRY_IDX(virt);

    table = current_pd;
    entry = &table->entries[table_idx];

    // Check if the table is mapped
    if (!(*entry & VM_PROT_PRESENT)) {
        unmapped += PAGE_SIZE * (1024 - entry_idx);
        table_idx++;
        entry_idx = 1024; // Skip to next table
    }
    table = &current_pts[table_idx];

    while (unmapped < size) {

        // Move to next table if needed
        if (entry_idx > 1023) {
            entry_idx = 0;
            table_idx++;
            entry = &current_pd->entries[table_idx];

            // Check if the table is mapped if not, skip it
            if (!(*entry & VM_PROT_PRESENT)) {
                unmapped += PAGE_SIZE * 1024;
                entry_idx = 1024; // Skip to next table
                continue;
            }
            table = &current_pts[table_idx];
        }
        entry = &table->entries[entry_idx];
        if (!(*entry & VM_PROT_PRESENT)) {
            // Not mapped, nothing to unmap
            return;
        }

        uint32_t phys = *entry & 0xFFFFF000;
        *entry = 0; // Clear the entry

        unmapped += PAGE_SIZE;
        entry_idx++;
    }
}

void vmm_unmap_tables(uint32_t table_idx_start, uint32_t table_count) {
    page_table_t *table;
    page_entry_t *entry;

    for (uint32_t i = 0; i < table_count; i++) {
        uint32_t table_idx = table_idx_start + i;

        table = current_pd;
        entry = &table->entries[table_idx];
        vmm_unmap((void *)(table_idx << 22), PAGE_SIZE * 1024);

        if (*entry & VM_PROT_PRESENT) {
            page_table_t *pt = (page_table_t *)(*entry & 0xFFFFF000);
            pmm_free_page(pt);
            *entry = 0; // Clear the entry
        }
    }
}

void vmm_zero(uintptr_t phys) {
    void *virt = (void*)PAGE_TABLE_EDIT_ADDRESS;
    vmm_map(virt, phys, PAGE_SIZE, VM_PROT_READWRITE, VM_MAP_ZERO | VM_MAP_PHYS);
    vmm_unmap(virt, PAGE_SIZE);
}

uintptr_t vmm_resolve(void *virt) {
    uint32_t phys;
    uint32_t flags;
    if (page_table_get_map((uint32_t)virt, &phys, &flags))
        return 0;
    return phys | ((uint32_t)virt & 0xFFF);
}

page_table_t *get_current_page_directory_phys() {
    page_table_t *pd;
    asm volatile("mov %%cr3, %0" : "=r"(pd));
    return pd;
}

#include "kernel/memory/vmspace.h"
#include "kernel/memory/vm.h"
#include "kernel/memory/mm.h"
#include "kernel/memory/layout.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/panic.h"
#include "kernel/terminal.h"

#include "types/string.h"

static vm_space_t* current_space = NULL;
vm_space_t* kernel_space = NULL;

void vm_space_init(void) {
    current_space = kmalloc(sizeof(vm_space_t));
    kernel_space = current_space;
    if (!current_space) PANIC("vm_space_init: Out of memory");
    current_space->page_directory = get_current_page_directory_phys();

    // Set up kernel_space and bootsector mappings
    vmm_map((void*)0x00000000, 0x00000000, 0x100000, VM_PROT_READWRITE, VM_MAP_PHYS | VM_MAP_FORCE);
}

vm_space_t *vm_space_create(void) {
    page_table_t *pd = pmm_alloc_page();
    if (!pd) return NULL;
    vm_space_t *vm = (vm_space_t*)kmalloc(sizeof(vm_space_t));
    if (!vm) {
        pmm_free_page(pd);
        return NULL;
    }
    vm->page_directory = pd;

    // Initialize page directory
    page_table_t* edit_pd = (page_table_t *)PAGE_TABLE_EDIT_ADDRESS;
    vmm_map(edit_pd, (uintptr_t)pd, PAGE_SIZE, VM_PROT_READWRITE, VM_MAP_PHYS | VM_MAP_ZERO);

    uintptr_t phys = vmm_resolve(edit_pd);

    // Copy kernel mappings
    for (uint32_t i = 0; i < 255; i++)
        edit_pd->entries[i + 768] = kernel_pd_entries[i];   

    // Add recursive mapping
    edit_pd->entries[1023] = (page_entry_t)pd | VM_PROT_PRESENT | VM_PROT_READWRITE;

    // Unmap the edit page directory
    vmm_unmap(edit_pd, PAGE_SIZE);

    return vm;
}

void vm_space_destroy(vm_space_t *space) {
    if (!space || current_space == space) return;
    page_table_t *pd = (page_table_t *)space->page_directory;
    for (uint32_t i = 0; i < 1024; i++) {
        if (pd->entries[i] & VM_PROT_PRESENT) {
            page_table_t *pt = (page_table_t *)PAGE_TABLES_ADDRESS + (i << 12);
            for (uint32_t j = 0; j < 1024; j++) {
                if (pt->entries[j] & VM_PROT_PRESENT) {
                    uint32_t phys = pt->entries[j] & 0xFFFFF000;
                    pmm_free_page((void *)phys);
                }
            }
            pmm_free_page(pt);
        }
    }
    pmm_free_page(pd);
    kfree(space);
}

void vm_space_map(vm_space_t *space, void *virt, uintptr_t phys, size_t size, int prot, int flags) {
    if (!space) return;
    vm_space_t *old_space = vm_space_switch(space);

    vmm_map(virt, phys, size, prot, flags);

    vm_space_switch(old_space);
}

void vm_space_unmap(vm_space_t *space, void *virt, size_t size) {
    if (!space) return;
    vm_space_t *old_space = vm_space_switch(space);

    vmm_unmap(virt, size);

    vm_space_switch(old_space);
}

vm_space_t* vm_space_switch(vm_space_t *space) {
    if (!space) return NULL;
    vm_space_t* old_space = current_space;
    page_table_t *pd = (page_table_t *)space->page_directory;
    current_space = space;
    asm volatile("mov %0, %%cr3" : : "r"(pd));
    return old_space;
}



#ifndef VM_TEMP_H
#define VM_TEMP_H

#include "types.h"

#define VM_TEMP_GUARD __attribute__((cleanup(vm_temp_cleanup)))

void* vm_temp_map_page(paddr_t phys_addr, vm_prot_t prot);
void  vm_temp_unmap_page(void* virt_addr);

static void vm_temp_cleanup(void* addr_ptr)
{
    void** addr = (void**)addr_ptr;
    if (*addr) {
        vm_temp_unmap_page(*addr);
    }
}

#endif // VM_TEMP_H

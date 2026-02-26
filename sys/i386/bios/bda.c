#include "bda.h"
#include <vm/vm_map.h>
#include <vm/layout.h>
#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

#include <string.h>

bda_t* bda = NULL;
ebda_t* ebda = NULL;

int load_bda() {
    // Allocate memory for BDA
    bda = (bda_t*)kmalloc(sizeof(bda_t));
    if (!bda) return ENOMEM;

    // Map and read BDA
    vm_map_region(CURRENT_VM_SPACE, BDA_PHYSICAL_ADDRESS, BDA_PHYSICAL_ADDRESS, sizeof(bda_t),
             VM_PROT_READ, VM_MAP_PHYS);
    memcpy(bda, (void*)BDA_PHYSICAL_ADDRESS, sizeof(bda_t));
    
    if (!bda->ebda_segment) return 0;

    uintptr_t ebda_phys = ((uintptr_t)(bda->ebda_segment) << 4);
    ebda_t* ebda_temp = (ebda_t*)ebda_phys;

    // Map first 4KB to read size
    vm_map_region(CURRENT_VM_SPACE, ebda_phys, ebda_phys, 4096,
             VM_PROT_READ, VM_MAP_PHYS | VM_MAP_FORCE);
    // Read EBDA size
    vm_map_region(CURRENT_VM_SPACE, ebda_phys, ebda_phys, ebda_temp->size_kb * 1024,
             VM_PROT_READ, VM_MAP_PHYS | VM_MAP_FORCE);

    // Allocate memory and copy EBDA
    ebda = kmalloc(ebda_temp->size_kb * 1024);
    if (!ebda) {
        vm_unmap_region(CURRENT_VM_SPACE, ebda_phys, ebda_temp->size_kb * 1024);
        return ENOMEM;
    }
    memcpy(ebda, (void*)ebda_phys, ebda_temp->size_kb * 1024);
    vm_unmap_region(CURRENT_VM_SPACE, ebda_phys, ebda->size_kb * 1024);

    return 0;
}

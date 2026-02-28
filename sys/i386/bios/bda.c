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
    // Map and read BDA
    bda = vm_map_device(BDA_PHYSICAL_ADDRESS, sizeof(bda_t), VM_PROT_READ, VM_REG_F_NONE);
    if (IS_ERR(bda)) return ERR_PTR(bda);
    
    if (!bda->ebda_segment) return 0;

    uintptr_t ebda_phys = ((uintptr_t)(bda->ebda_segment) << 4);

    // Map first 4KB to read size
    ebda_t* ebda_temp = vm_map_device(ebda_phys, PAGE_SIZE, VM_PROT_READ, VM_REG_F_NONE);
    if (IS_ERR(ebda_temp)) return (int)ebda_temp;

    // Read EBDA size
    ebda = vm_map_device(ebda_phys, ebda_temp->size_kb * 1024, VM_PROT_READ, VM_REG_F_NONE);
    vm_unmap_device(ebda_temp, PAGE_SIZE);
    if (IS_ERR(ebda)) return (int)ebda;

    return 0;
}

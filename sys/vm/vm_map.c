#include "vm_map.h"
#include "kmalloc.h"
#include "types.h"
#include "vm_phys.h"
#include "vm_region.h"

#include <kern/errno.h>

int vm_map_region(vm_space_t *space, vaddr_t virt, paddr_t phys, size_t size, vm_prot_t prot, vm_flags_t flags) {
    vm_region_t* region = vm_create_region(space, virt, size, prot, flags);
    if (IS_ERR(region)) return ERR_PTR(region);

    size_t offset;
    for (offset = 0; offset < size; offset += PAGE_SIZE) {
        paddr_t p_add;
        if (flags & VM_MAP_PHYS) {
            // TODO: Handle the case where the physical address is already mapped to a different virtual address in the same space, we should be able to just reuse the existing mapping instead of creating a new one and potentially causing fragmentation in the page tables
            vm_phys_alloc_specific_page(phys + offset);
            p_add = phys + offset;
        } else {
            p_add = vm_phys_alloc_page();
        }
        if (!p_add) goto cleanup;
        int ret = pmap_enter(&space->arch, virt + offset, p_add, prot, VM_MAP_PHYS);
        if (IS_ERR(ret)) return ret;
    }

    return 0;

cleanup:
    vm_region_dec_ref(region); // This will free the region and its object since their ref counts are 1
    pmap_remove(&space->arch, virt, virt + offset);

    return -ENOMEM;
}

int vm_unmap_region(vm_space_t *space, uintptr_t addr, size_t size) {
    vm_region_t* region = vm_region_lookup(space, addr);
    if (!region) return -ENOENT;

    if (region->base != addr || region->end != addr + size) return -EINVAL;

    list_remove(&region->node);
    kfree(region);

    for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        pmap_remove(&space->arch, addr + offset, addr + offset + PAGE_SIZE);
    }

    return 0;
}

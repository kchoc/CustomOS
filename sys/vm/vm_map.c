#include "vm_map.h"
#include "types.h"
#include "vm_page.h"
#include "vm_region.h"
#include "vm_space.h"

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/spinlock.h>
#include <kern/terminal.h>

#include <string.h>

int vm_map(vm_space_t* space, vaddr_t* virt, size_t size, vm_prot_t prot, vm_region_flags_t flags,
           vm_object_t* object, vm_ooffset_t offset, vm_map_flags_t map_flags)
{
    if (!space || !virt || size == 0)
        return -EINVAL;

    vm_region_t* region =
        vm_region_create(space, virt, size, object, offset, prot, flags, map_flags);
    if (IS_ERR(region))
        return PTR_ERR(region);

    return 0;
}

int vm_map_anon(vm_space_t* space, uintptr_t* virt, size_t size, vm_prot_t prot,
                vm_region_flags_t flags, vm_map_flags_t map_flags)
{
    return vm_map(space, virt, size, prot, flags, NULL, 0, map_flags);
}

int vm_unmap(vm_space_t* space, uintptr_t virt, size_t size)
{
    if (!space || size == 0)
        return -EINVAL;

    vm_region_free_range(space, virt, size);
    return 0;
}

int vm_protect(vm_space_t* space, uintptr_t virt, size_t size, vm_prot_t prot)
{
    if (!space || size == 0)
        return -EINVAL;

    vm_region_protect_range(space, virt, size, prot);
    return 0;
}

void* kvm_map(size_t size, vm_prot_t prot, vm_region_flags_t flags)
{
    if (size == 0)
        return ERR_PTR(-EINVAL);

    size_t  aligned_size = PAGE_ALIGN_UP(size);
    vaddr_t virt         = 0;
    int     ret = vm_map(kvm_space, &virt, aligned_size, prot, VM_REG_F_KERNEL | flags, NULL, 0, 0);
    if (IS_ERR(ret))
        return ERR_PTR(ret);

    return (void*)virt;
}

void kvm_unmap(void* virt, size_t size)
{
    if (!virt || size == 0)
        return;

    size_t aligned_size = PAGE_ALIGN_UP(size);
    vm_unmap(kvm_space, (uintptr_t)virt, aligned_size);
}

void* vm_map_device(paddr_t addr, size_t size, vm_prot_t prot, vm_region_flags_t flags)
{
    paddr_t end_phys = addr + size;
    if (size == 0 || end_phys < addr)
        return ERR_PTR(-EINVAL);

    paddr_t aligned_phys = PAGE_ALIGN_DOWN(addr);
    size_t  aligned_size = end_phys - PAGE_ALIGN_DOWN(addr);

    vaddr_t virt =
        (vaddr_t)kvm_map(aligned_size, prot, VM_REG_F_DEVICE | VM_REG_F_WIRED | VM_REG_F_NOCACHE);
    if (IS_ERR(virt))
        return ERR_PTR(-ENOMEM);

    for (paddr_t phys = aligned_phys; phys < end_phys; phys += PAGE_SIZE) {
        int ret =
            pmap_enter(kvm_space->pmap, virt + (phys - aligned_phys), phys, prot, PMAP_FLAG_NONE);
        if (IS_ERR(ret)) {
            kvm_unmap((void*)virt, phys - aligned_phys);
            return ERR_PTR(ret);
        }
    }

    return (void*)(virt + (addr - aligned_phys));
}

void vm_unmap_device(void* virt, size_t size)
{
    vaddr_t aligned_virt = PAGE_ALIGN_DOWN((vaddr_t)virt);
    vaddr_t end_virt     = PAGE_ALIGN_UP((vaddr_t)virt + size);

    kvm_unmap((void*)aligned_virt, end_virt - aligned_virt);
}

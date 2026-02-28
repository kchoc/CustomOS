#include "vm_map.h"
#include "kmalloc.h"
#include "types.h"
#include "vm/types.h"
#include "vm_page.h"
#include "vm_phys.h"
#include "vm_region.h"
#include "vm_space.h"

#include <kern/panic.h>
#include <kern/errno.h>

// TODO: Add flags
int vm_map(vm_space_t* space, vaddr_t* virt, size_t size, vm_prot_t prot, vm_region_flags_t flags, vm_object_t* object, vm_ooffset_t offset) {
    vm_region_t* region = vm_create_region(space, virt, size, object, offset, prot, flags);
    if (IS_ERR(region)) return ERR_PTR(region);
    return 0;
}

int vm_map_anon(vm_space_t *space, uintptr_t* virt, size_t size, vm_prot_t prot, vm_region_flags_t flags) {
    return vm_map(space, virt, size, prot, flags, NULL, 0);
}

int vm_unmap(vm_space_t *space, uintptr_t virt, size_t size) {
    vm_region_t* region = vm_region_lookup(space, virt);
    if (!region) return -ENOENT;

    //TODO: Handle unmapping of partial regions (split the region if necessary)
    if (region->base != virt || region->end != virt + size) return -EINVAL;

    vm_region_dec_ref(region); // This will free the region if this was the last reference

    return 0;
}

int vm_protect(vm_space_t *space, uintptr_t virt, size_t size, vm_prot_t prot) {
    vm_region_t* region = vm_region_lookup(space, virt);
    if (!region) return -ENOENT;

    //TODO: Handle changing protection of partial regions (split the region if necessary)
    if (region->base != virt || region->end != virt + size) return -EINVAL;

    region->prot = prot;

    for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        pmap_protect(space->arch, virt + offset, virt + offset + PAGE_SIZE, prot);
    }

    return 0;
}


void* vm_map_device(paddr_t addr, size_t size, vm_prot_t prot, vm_region_flags_t flags) {
    paddr_t aligned_phys = PAGE_ALIGN_DOWN(addr);
    paddr_t end_phys = PAGE_ALIGN_UP(addr + size);

    size_t aligned_size = end_phys - aligned_phys;

    vaddr_t virt = (vaddr_t)kvm_alloc(aligned_size, prot, VM_REG_F_WIRED | (flags & VM_REG_F_NOCACHE ? VM_REG_F_NOCACHE : 0));
    if (IS_ERR(virt)) return ERR_PTR(-ENOMEM);

    int pmap_flags = PMAP_FLAG_WIRED;
    if (flags & VM_DEV_NOCACHE) pmap_flags |= PMAP_FLAG_NOCACHE;

    for (paddr_t phys = aligned_phys; phys < end_phys; phys += PAGE_SIZE) {
        // TODO: Consider bookkeeping in object for device mappings to allow for proper unmapping and cleanup
        int ret = pmap_enter(kernel_vm_space->arch, virt + (phys - aligned_phys), phys, prot, pmap_flags);
        if (IS_ERR(ret)) {
            kvm_unmap((void*)virt, phys - aligned_phys);
            return ERR_PTR(ret);
        }
    }

    return (void*)(virt + (addr - aligned_phys));
}

void vm_unmap_device(void* virt, size_t size) {
    vaddr_t aligned_virt = PAGE_ALIGN_DOWN((vaddr_t)virt);
    vaddr_t end_virt = PAGE_ALIGN_UP((vaddr_t)virt + size);

    for (vaddr_t offset = 0; offset < end_virt - aligned_virt; offset += PAGE_SIZE) {
        pmap_remove(kernel_vm_space->arch, aligned_virt + offset, aligned_virt + offset + PAGE_SIZE);
    }

    kvm_free((void*)aligned_virt, end_virt - aligned_virt);
}

void* kvm_alloc(size_t size, vm_prot_t prot, vm_region_flags_t flags) {
    size_t aligned_size = PAGE_ALIGN_UP(size);
    vaddr_t virt = VM_REGION_ALLOCATE_ADDR;
    int ret = vm_map(kernel_vm_space, &virt, aligned_size, prot, VM_REG_F_KERNEL | flags, NULL, 0);
    
    if (IS_ERR(ret)) return ERR_PTR(ret);

    return (void*)virt;
}

void kvm_free(void* virt, size_t size) {
    size_t aligned_size = PAGE_ALIGN_UP(size);
    vm_unmap(kernel_vm_space, (uintptr_t)virt, aligned_size);
}

void* kvm_map(size_t size, vm_prot_t prot, vm_region_flags_t flags) {
    size_t aligned_size = PAGE_ALIGN_UP(size);
    vaddr_t kva = (vaddr_t)kvm_alloc(aligned_size, prot, flags);

    if (IS_ERR(kva)) return ERR_PTR(kva);

    int pmap_flags = PMAP_FLAG_WIRED;
    if (flags & VM_REG_F_NOCACHE) pmap_flags |= PMAP_FLAG_NOCACHE;

    vm_object_t* obj = vm_region_lookup(kernel_vm_space, kva)->object;
    for (size_t offset = 0; offset < aligned_size; offset += PAGE_SIZE) {
        vm_page_t* page = vm_page_allocate(obj, offset);
        if (IS_ERR(page)) {
            kvm_unmap((void*)kva, offset);
            return page;
        }
        int ret = pmap_enter(kernel_vm_space->arch, kva + offset, page->phys_addr, prot, pmap_flags);
        if (IS_ERR(ret)) {
            kvm_unmap((void*)kva, offset);
            return ERR_PTR(ret);
        }

        void* page_virt = (void*)(kva + offset);
        memset(page_virt, 0, PAGE_SIZE);
    }

    return (void*)kva;
}

void kvm_unmap(void* virt, size_t size) {
    size_t aligned_size = PAGE_ALIGN_UP(size);
    kvm_free(virt, aligned_size);
}


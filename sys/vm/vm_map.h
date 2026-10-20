#ifndef VM_MAP_H
#define VM_MAP_H

#include "types.h"
#include <stddef.h>

int vm_map(vm_space_t* space, vaddr_t* virt, size_t size, vm_prot_t prot, vm_region_flags_t flags,
           vm_object_t* object, vm_ooffset_t offset, vm_map_flags_t map_flags);
int vm_unmap(vm_space_t* space, vaddr_t virt, size_t size);
int vm_map_anon(vm_space_t* space, vaddr_t* virt, size_t size, vm_prot_t prot,
                vm_region_flags_t flags, vm_map_flags_t map_flags);

int vm_protect(vm_space_t* space, vaddr_t virt, size_t size, vm_prot_t prot);

void* vm_map_device(paddr_t phys, size_t size, vm_prot_t prot, vm_region_flags_t flags);
void  vm_unmap_device(void* virt, size_t size);

void* kvm_map(size_t size, vm_prot_t prot, vm_region_flags_t flags);
void  kvm_unmap(void* virt, size_t size);

#endif // VM_MAP_H

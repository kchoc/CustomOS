#ifndef VM_MAP_H
#define VM_MAP_H

#include "vm_space.h"

#include "types.h"
#include <stddef.h>

typedef struct vm_space vm_space_t;
typedef struct vm_object vm_object_t;

int vm_map(vm_space_t* space, vaddr_t* virt, size_t size, vm_prot_t prot, vm_region_flags_t flags,
	vm_object_t* object, vm_ooffset_t offset);
int vm_unmap(vm_space_t* space, vaddr_t virt, size_t size);
int vm_protect(vm_space_t* space, vaddr_t virt, size_t size, vm_prot_t prot);

int vm_map_anon(vm_space_t* space, vaddr_t* virt, size_t size, vm_prot_t prot, vm_region_flags_t flags);

void* vm_map_device(paddr_t phys, size_t size, vm_prot_t prot, vm_region_flags_t flags);
void vm_unmap_device(void* virt, size_t size);

void* kvm_alloc(size_t size, vm_prot_t prot, vm_region_flags_t flags);
void kvm_free(void* virt, size_t size);

void* kvm_alloc_wired(size_t size);
void kvm_free_wired(void* virt, size_t size);

// Allocates a physically contiguous block of memory suitable for DMA (Direct Memory Access) operations.
void* kvm_alloc_dma(size_t size, paddr_t* phys);

void* kvm_map(size_t size, vm_prot_t prot, vm_region_flags_t flags);
void kvm_unmap(void* virt, size_t size);

#endif // VM_MAP_H

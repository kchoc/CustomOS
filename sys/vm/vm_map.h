#ifndef VM_MAP_H
#define VM_MAP_H

#include "vm_space.h"
#include <kern/pcpu.h>

#include "types.h"
#include <stddef.h>

typedef struct vm_space vm_space_t;
typedef struct vm_object vm_object_t;

#define CURRENT_VM_SPACE PCPU_GET(vmspace)

int vm_map(vm_space_t* space, vaddr_t addr, size_t size, vm_object_t* object,
	vm_ooffset_t offset, vm_prot_t prot, vm_flags_t flags);
int vm_unmap(vm_space_t* space, vaddr_t addr, size_t size);
int vm_protect(vm_space_t* space, vaddr_t addr, size_t size, vm_prot_t prot);

#endif // VM_MAP_H

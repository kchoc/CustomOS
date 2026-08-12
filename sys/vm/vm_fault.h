#ifndef VM_FAULT_H
#define VM_FAULT_H

#include "types.h"
#include "vm_space.h"

int vm_fault(vm_space_t* space, vaddr_t addr, vm_prot_t fault_type);

#endif // VM_FAULT_H

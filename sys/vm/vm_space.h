#ifndef VM_SPACE_H
#define VM_SPACE_H

#include <machine/pmap.h>

#include <kern/spinlock.h>

#include <list.h>

typedef struct vm_region vm_region_t;

typedef struct vm_space {
    list_t  regions;
    pmap_t* arch; // Architecture-specific data (e.g. page directory)
    uint32_t fault_count; // Number of active faults occurring in this vm_space (used to prevent destruction while faults are active) 
    spinlock_t regions_lock; // Lock for synchronizing access to the regions list fault_count must be 0 before proceeding with alterations.
} vm_space_t;

extern vm_space_t kernel_vm_space;

int         kvm_space_init();
vm_space_t* vm_space_create();
void        vm_space_clean(vm_space_t* space);
vm_space_t* vm_space_fork(vm_space_t* parent);
void        vm_space_destroy(vm_space_t* space);
void        vm_space_activate(vm_space_t* space);
void        vm_space_debug(vm_space_t* space);

#endif

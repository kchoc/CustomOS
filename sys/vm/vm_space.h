#ifndef VM_SPACE_H
#define VM_SPACE_H

#include <machine/pmap.h>

#include <kern/rwlock.h>
#include <kern/spinlock.h>

#include <list.h>

typedef struct vm_region vm_region_t;

typedef struct vm_space {
    list_t     regions;
    rwlock_t   regions_lock;   // Read-write lock for synchronizing access to the regions list
    pmap_t*    arch;           // Architecture-specific data (e.g. page directory)
    spinlock_t lifecycle_lock; // Lock for synchronizing access to the lifecycle of the vm_space
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

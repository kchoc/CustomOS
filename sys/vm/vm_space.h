#ifndef VM_SPACE_H
#define VM_SPACE_H

#include "pmap.h"

#include <kern/rwlock.h>
#include <kern/spinlock.h>

#include <list.h>

typedef struct vm_region vm_region_t;

typedef struct vm_space {
    list_t     regions;
    rwlock_t   regions_lock;
    pmap_t*    pmap;
    spinlock_t lifecycle_lock;
} vm_space_t;

extern vm_space_t kvm_space_store;
#define kvm_space (&kvm_space_store)

int         kvm_space_init();
vm_space_t* vm_space_create();
void        vm_space_clean(vm_space_t* space);
vm_space_t* vm_space_fork(vm_space_t* parent);
void        vm_space_destroy(vm_space_t* space);
void        vm_space_activate(vm_space_t* space);
void        vm_space_debug(vm_space_t* space);

#endif // VM_SPACE_H

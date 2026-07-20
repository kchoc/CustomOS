#ifndef VM_REGION_H
#define VM_REGION_H

#include "types.h"
#include "vm_object.h"

#include <kern/rwlock.h>

#include <list.h>

typedef struct vm_object vm_object_t;
typedef struct vm_space  vm_space_t;

typedef struct vm_region {
    list_node_t node;

    vaddr_t base;
    vaddr_t end;

    vm_prot_t         prot;
    vm_region_flags_t flags;

    rwlock_t lock; // Protects the region's metadata

    vm_object_t* object;
    size_t       offset;
} vm_region_t;

#define list_node_to_region(nptr)    container_of(nptr, vm_region_t, node)
#define vm_space_from_region(region) container_of((region)->node.list, vm_space_t, regions)
#define GET_NEXT_REGION(region)      list_node_to_region((region)->node.next)
#define GET_PREV_REGION(region)      list_node_to_region((region)->node.prev)

/* Returns the region that contains the address addr, or NULL if no such region exists */
vm_region_t* vm_region_lookup(vm_space_t* space, uintptr_t addr, lock_func_t lock_func);
/* Returns the first region that overlaps with the range [addr, addr + size) */
vm_region_t* vm_region_lookup_range(vm_space_t* space, uintptr_t addr, size_t size);
void         vm_region_free_range(vm_space_t* space, uintptr_t addr, size_t size);
void vm_region_protect_range(vm_space_t* space, uintptr_t addr, size_t size, vm_prot_t new_prot);
vm_region_t* vm_region_create(vm_space_t* space, vaddr_t* addr, size_t size, vm_object_t* object,
                              vm_ooffset_t offset, vm_prot_t prot, vm_region_flags_t flags,
                              vm_map_flags_t map_flags);
vm_region_t* vm_region_fork(vm_region_t* parent);
vm_region_t* vm_region_insert(vm_space_t* space, vm_region_t* new_region);
void         vm_region_destroy(vm_region_t* region);

#endif // VM_REGION_H

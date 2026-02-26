#ifndef VM_REGION_H
#define VM_REGION_H

#include "vm_object.h"

#include "list.h"
#include "types.h"

typedef struct vm_object vm_object_t;
typedef struct vm_space vm_space_t;

typedef struct vm_region {
	list_node_t node;
	vaddr_t base;
	vaddr_t end;
	vm_prot_t prot;
	vm_flags_t flags;
	int ref_count;

	vm_object_t* object;
	size_t offset;
} vm_region_t;

#define list_node_to_region(nptr) ((vm_region_t *)((char *)(nptr) - offsetof(vm_region_t, node)))
#define vm_space_from_region(region) ((vm_space_t *)(region->node.list - offsetof(vm_space_t, regions)))
#define GET_NEXT_REGION(region) list_node_to_region((region)->node.next)
#define GET_PREV_REGION(region) list_node_to_region((region)->node.prev)

void vm_region_inc_ref(vm_region_t *region);
void vm_region_dec_ref(vm_region_t *region);

/* Returns the region that contains the address addr, or NULL if no such region exists */
vm_region_t* vm_region_lookup(vm_space_t *space, uintptr_t addr);
/* Returns the first region that overlaps with the range [addr, addr + size) */
vm_region_t* vm_region_lookup_range(vm_space_t *space, uintptr_t addr, size_t size);

bool vm_region_overlaps(vm_region_t *region, uintptr_t addr, size_t size);

vm_region_t* vm_region_create(vm_space_t *space, uintptr_t addr, size_t size, vm_prot_t prot, vm_flags_t flags);
vm_region_t* vm_region_insert(vm_space_t *space, vm_region_t *new_region);

vm_region_t* vm_region_split(vm_region_t *region, uintptr_t addr);
vm_region_t* vm_region_merge(vm_region_t *region, vm_region_t *other);
vm_region_t* vm_region_protect(vm_region_t *region, vm_prot_t new_prot);
vm_region_t* vm_region_flag(vm_region_t *region, vm_flags_t flags);
vm_region_t* vm_region_rebase(vm_region_t *region, uintptr_t new_addr);
vm_region_t* vm_region_resize(vm_region_t *region, uintptr_t new_end);
vm_region_t* vm_region_remap(vm_region_t *region, uintptr_t new_addr, size_t new_size);
vm_region_t* vm_region_destroy(vm_region_t *region);

#endif // VM_REGION_H

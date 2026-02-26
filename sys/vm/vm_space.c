#include "vm_space.h"
#include "vm_region.h"

#include "kmalloc.h"
#include "libkern/list.h"
#include <kern/errno.h>

vm_space_t* vm_space_create() {
	vm_space_t* space = kmalloc(sizeof(vm_space_t));
	if (!space) return ERR_PTR(-ENOMEM);

	list_init(&space->regions, 0);
	space->arch = pmap_create();
	space->ref_count = 1;

	return space;
}

vm_space_t* vm_space_fork(vm_space_t* parent) {
	if (!parent) return ERR_PTR(-EINVAL);

	vm_space_t* child = kmalloc(sizeof(vm_space_t));
	if (!child) return ERR_PTR(-ENOMEM);

	// Copy the regions list (shallow copy)
	list_init(&child->regions, 0);
	for (list_node_t* node = parent->regions.head; node; node = node->next) {
		vm_region_t* region = list_node_to_region(node);
		list_push_head(&child->regions, &region->node); // Note: This is a shallow copy
	}

	child->arch = pmap_fork(parent->arch); // Fork the architecture-specific data
	child->ref_count = 1;

	return child;
}

void vm_space_destroy(vm_space_t* space) {
	if (!space) return;

	// Decrement reference counts for all regions and their objects (this will free them if this was the last reference)
	list_node_t* node;
	list_for_each(node, &space->regions) {
		vm_region_t* region = list_node_to_region(node);
		vm_region_dec_ref(region); // This will free the region and its object if this was the last reference
	}

	// Free architecture-specific data
	pmap_destroy(space->arch);

	kfree(space);
}

void vm_space_activate(vm_space_t *space) {
	if (!space) return;
	pmap_activate(space->arch);
}

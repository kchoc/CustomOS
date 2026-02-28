#include "vm_space.h"
#include "types.h"
#include "vm_map.h"
#include "vm_phys.h"
#include "vm_region.h"

#include "kmalloc.h"
#include <kern/pcpu.h>
#include "libkern/list.h"
#include <kern/errno.h>

vm_space_t* kernel_vm_space = NULL;

int kvm_space_init() {
	kernel_vm_space = vm_space_create();
	if (IS_ERR(kernel_vm_space)) return (int)kernel_vm_space;
	vm_phys_free_page((paddr_t)(kernel_vm_space->arch->pd)); // Free the page allocated for the kernel page directory since we'll be using the one identity mapped by the bootloader
	kernel_vm_space->arch->pd = *current_pd_addr; // Use the page directory set up by the bootloader
	pcpus[0].vmspace = kernel_vm_space;

	// Since this is the first kvm call, the start is at 0xC0000000, so we can allocate that
	vaddr_t virt = KERNEL_BASE;
    int ret = vm_map_anon(kernel_vm_space, &virt, 0x00400000, VM_PROT_READ | VM_PROT_WRITE, VM_REG_F_KERNEL);
	return ret;
}

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

	vm_space_t* child = vm_space_create();
	if (!child) return ERR_PTR(-ENOMEM);

	// Copy the regions list (shallow copy)
	for (list_node_t* node = parent->regions.head; node; node = node->next) {
		vm_region_t* reference_region = list_node_to_region(node);
		vm_region_t* child_region = vm_region_fork(reference_region);
		if (IS_ERR(child_region)) {
			vm_space_destroy(child); // Clean up the child space and all regions created so far
			return ERR_PTR(child_region);
		}
		// Insert the child region into the child's region list
		list_push_tail(&child->regions, &child_region->node);
	}

	return child;
}

void vm_space_destroy(vm_space_t* space) {
	if (!space) return;

	// Decrement reference counts for all regions and their objects (this will free them if this was the last reference)
	while (space->regions.head) {
	    vm_region_t* region = list_node_to_region(space->regions.head);
	    list_remove(&region->node);
	    vm_region_dec_ref(region);
	}

	// Free architecture-specific data
	pmap_destroy(space->arch);

	kfree(space);
}

void vm_space_activate(vm_space_t *space) {
	if (!space) return;
	pmap_activate(space->arch);
}

#include "vm_space.h"
#include "machine/pmap.h"
#include "vm_map.h"
#include "vm_region.h"
#include "kmalloc.h"
#include "types.h"

#include <sys/pcpu.h>

#include <kern/panic.h>
#include <kern/terminal.h>
#include <kern/errno.h>

#include <list.h>

vm_space_t* kernel_vm_space = NULL;

int kvm_space_init() {
	kernel_vm_space = kmalloc(sizeof(vm_space_t));
	if (IS_ERR(kernel_vm_space)) return (int)kernel_vm_space;

	kernel_vm_space->ref_count = 1;
	list_init(&kernel_vm_space->regions, 0);
	kernel_vm_space->arch = kmalloc(sizeof(pmap_t));
	if (IS_ERR(kernel_vm_space->arch)) {
		kfree(kernel_vm_space);
		return (int)kernel_vm_space->arch;
	}

	kernel_vm_space->arch->pd = *current_pd_addr; // Use the page directory set up by the bootloader
	pcpus[0].vmspace = kernel_vm_space;

	// Since this is the first kvm call, the start is at 0xC0000000, so we can allocate that
	vaddr_t virt = KERNEL_BASE;
  int ret = vm_map_anon(kernel_vm_space, &virt, 0x00400000, VM_PROT_READ | VM_PROT_WRITE, VM_REG_F_KERNEL);
	if (IS_ERR(ret)) return ret;

	pmap_init();

    // TODO: Need to bookkeep the vm_pages from the bootloader
	return 0;
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

void vm_space_debug(vm_space_t *space) {
  if (!space) return;  

  printf("VM Space at %p\n", space);
  printf("  Ref Count: %d\n", space->ref_count);
  printf("  Regions:\n");
  list_node_t* node;
  list_for_each(node, &space->regions) {
    vm_region_t* region = list_node_to_region(node);
    printf("    Region at %p: base=0x%08x, end=0x%08x, prot=%d, flags=%d\n",
           region, region->base, region->end, region->prot, region->flags);
  }
  // pmap_debug(space->arch);
}


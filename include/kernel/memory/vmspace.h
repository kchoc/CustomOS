#ifndef VMSPACE_H
#define VMSPACE_H

#include "kernel/types.h"

typedef struct vm_space {
	void *page_directory; // Pointer to the page directory for this space (physical address)
} vm_space_t;

static vm_space_t* current_space;

void        vm_space_init(void);
vm_space_t* vm_space_create(void);
void        vm_space_destroy(vm_space_t *space);
void        vm_space_map(vm_space_t *space, void *virt, uintptr_t phys, size_t size, int prot, int flags);
void		    vm_space_unmap(vm_space_t *space, void *virt, size_t size);
void        vm_space_copy_mappings(vm_space_t *space, uintptr_t src_start, uintptr_t dest_start, size_t size, int prot);
vm_space_t* vm_space_switch(vm_space_t *space);

#endif // VMSPACE_H

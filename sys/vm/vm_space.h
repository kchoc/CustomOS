#ifndef VM_SPACE_H
#define VM_SPACE_H

#include <machine/pmap.h>
#include "list.h"

typedef struct vm_region vm_region_t;

typedef struct vm_space {
	list_t regions;
	pmap_t* arch; // Architecture-specific data (e.g. page directory)
	int ref_count;
} vm_space_t;

vm_space_t* vm_space_create();
vm_space_t* vm_space_fork(vm_space_t* parent);
void vm_space_destroy(vm_space_t* space);
void vm_space_activate(vm_space_t *space);

#endif
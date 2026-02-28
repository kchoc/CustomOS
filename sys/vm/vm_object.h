#ifndef VM_OBJECT_H
#define VM_OBJECT_H

#include "libkern/list.h"
#include "types.h"

#include "list.h"
#include <stddef.h>

struct vm_page;
struct vm_pager;

typedef enum vm_object_type {
	VM_OBJECT_ANON,
	VM_OBJECT_VNODE,
	VM_OBJECT_PHYS,
	VM_OBJECT_SWAP,
	VM_OBJECT_DEVICE,
	VM_OBJECT_SHADOW
} vm_object_type_t;

typedef struct vm_object {
	list_node_t node;
	vm_object_type_t type;
	int ref_count;

	struct vm_object* shadow; // Shadow object for copy-on-write
	vm_ooffset_t shadow_offset; // Offset within the shadow object

	struct vm_pager* pager; // Pager for handling page faults and backing storage
	list_t pages; // List of vm_page_t

	size_t size; // Size of the object in bytes
} vm_object_t;

inline bool vm_object_supports_cow(vm_object_type_t type) {
	return type == VM_OBJECT_ANON || type == VM_OBJECT_VNODE || type == VM_OBJECT_SHADOW || type == VM_OBJECT_SWAP;
}

void vm_object_inc_ref(vm_object_t *obj);
void vm_object_dec_ref(vm_object_t *obj);

vm_object_t* vm_object_create(vm_object_t* obj);
vm_object_t* vm_object_create_shadow(vm_object_t* parent, vm_ooffset_t offset);
void vm_object_add_page(vm_object_t* obj, size_t offset, vm_prot_t prot);
void vm_object_remove_page(vm_object_t* obj, size_t offset);

#endif // VM_OBJECT_H

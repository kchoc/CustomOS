#ifndef VM_PAGER_H
#define VM_PAGER_H

#include "types.h"
#include <stddef.h>

typedef struct vm_object vm_object_t;
typedef struct vm_page vm_page_t;

typedef struct vm_pager_ops {
	int (*get_page)(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page);
	int (*put_page)(vm_object_t* obj, vm_page_t* page);
	bool (*has_page)(vm_object_t* obj, vm_ooffset_t offset);
} vm_pager_ops_t;

typedef struct vm_pager {
	vm_pager_ops_t* ops;
	void* data; // Pager-specific data (e.g. file handle for vnode pager)
} vm_pager_t;

#endif // VM_PAGER_H

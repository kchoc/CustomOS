#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "list.h"
#include "types.h"
#include "vm_object.h"

typedef enum vm_page_flags {
	VM_PAGE_FLAG_FREE,
	VM_PAGE_FLAG_ACTIVE,
	VM_PAGE_FLAG_INACTIVE,
	VM_PAGE_FLAG_WIRED
} vm_page_flags_t;

typedef struct vm_page {
	list_node_t node;
	paddr_t phys_addr; // Physical address of the page
	vm_ooffset_t offset; // Offset within the object

	vm_page_flags_t state;

	bool dirty; // Whether the page has been modified
	bool busy_lock;  // Whether the page is currently being read/written to disk
	int ref_count; // Reference count for shared pages
	bool wired; // Whether the page is wired (cannot be paged out)
} vm_page_t;

inline vm_page_t* list_node_to_page(list_node_t* node) {
	return (vm_page_t *)((char *)(node) - offsetof(vm_page_t, node));
}

inline vm_object_t *vm_page_get_object(vm_page_t* page) {
	return (vm_object_t *)(page->node.list - offsetof(vm_object_t, pages));
}

vm_page_t* vm_page_lookup(vm_object_t *obj, size_t offset);
vm_page_t* vm_page_bookmark(vm_object_t *obj, size_t offset, paddr_t phys_addr);
vm_page_t* vm_page_allocate(vm_object_t *obj, size_t offset);
void vm_page_free(vm_page_t *page);

#endif // VM_PAGE_H

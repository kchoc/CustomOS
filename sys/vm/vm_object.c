#include "vm_object.h"
#include "vm_page.h"

void vm_object_inc_ref(vm_object_t *obj) {
	__sync_fetch_and_add(&obj->ref_count, 1);
}

void vm_object_dec_ref(vm_object_t *obj) {
	if (__sync_fetch_and_sub(&obj->ref_count, 1) == 0) {
		// Free the object and its pages if this was the last reference
		list_node_t *node;
		list_for_each(node, &obj->pages) {
			vm_page_t *page = list_node_to_page(node);
			vm_page_free(page);
		}
		kfree(obj);
	}
}
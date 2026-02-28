#include "vm_object.h"
#include "vm_page.h"

#include "kmalloc.h"
#include <kern/errno.h>

#include "list.h"

void vm_object_inc_ref(vm_object_t *obj) {
	__sync_fetch_and_add(&obj->ref_count, 1);
}

void vm_object_dec_ref(vm_object_t *obj) {
	if (__sync_sub_and_fetch(&obj->ref_count, 1) == 0) {
		// Free the object and its pages if this was the last reference
		while (obj->pages.head) {
			vm_page_t* page = list_node_to_page(obj->pages.head);
			vm_page_free(page);
		}
		kfree(obj);
	}
}

vm_object_t* vm_object_create(vm_object_t *obj) {
	vm_object_t *new_obj = kmalloc(sizeof(vm_object_t));
	if (!new_obj) return ERR_PTR(-ENOMEM);

	new_obj->type = obj ? obj->type : VM_OBJECT_ANON;
	new_obj->ref_count = 1;
	new_obj->shadow = obj;
	new_obj->shadow_offset = 0;
	new_obj->pager = NULL;
	list_init(&new_obj->pages, 0);
	new_obj->size = obj ? obj->size : 0;

	if (obj) {
		vm_object_inc_ref(obj);
	}

	return new_obj;
}

vm_object_t* vm_object_create_shadow(vm_object_t *shadow, vm_ooffset_t offset) {
	vm_object_t *new_obj = kmalloc(sizeof(vm_object_t));
	if (!new_obj) return ERR_PTR(-ENOMEM);

	new_obj->type = VM_OBJECT_SHADOW;
	new_obj->ref_count = 1;
	new_obj->shadow = shadow;
	new_obj->shadow_offset = offset;
	new_obj->pager = NULL;
	list_init(&new_obj->pages, 0);
	new_obj->size = shadow ? shadow->size - offset : 0;

	if (shadow) {
		vm_object_inc_ref(shadow);
	}

	return new_obj;
}
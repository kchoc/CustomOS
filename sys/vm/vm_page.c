#include "vm_page.h"
#include "vm_phys.h"
#include "vm_object.h"
#include "kmalloc.h"

#define list_node_to_page(node) ((vm_page_t *)((char *)(node) - offsetof(vm_page_t, node)))

vm_page_t* vm_page_lookup(vm_object_t *obj, size_t offset) {
    list_node_t* node;
    list_for_each(node, &obj->pages) {
		vm_page_t* page = list_node_to_page(node);
		if (page->offset == offset) {
			return page;
		}
	}

    return NULL;
}

vm_page_t *vm_page_allocate(vm_object_t *obj, size_t offset) {
    uintptr_t phys = vm_phys_alloc_page();  // your bitmap allocator

    if (!phys)
        return NULL;

    struct vm_page *page = kmalloc(sizeof(*page));
    page->phys_addr = phys;
    page->offset    = offset;

    list_push_head(&obj->pages, &page->node);

    return page;
}

void vm_page_free(vm_page_t *page) {
    vm_phys_free_page(page->phys_addr);  // your bitmap allocator
    list_remove(&page->node);
    kfree(page);
}

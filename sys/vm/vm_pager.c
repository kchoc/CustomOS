#include "vm_pager.h"

#include "kmalloc.h"
#include "vm_page.h"
#include "vm_phys.h"

#include <kern/errno.h>
#include <kern/spinlock.h>

#include <list.h>

vm_pager_t* vm_pager_create(vm_pager_ops_t* ops, void* data)
{
    vm_pager_t* pager = kmalloc(sizeof(vm_pager_t));
    if (!pager)
        return ERR_PTR(-ENOMEM);

    pager->ops  = ops;
    pager->data = data;

    return pager;
}

void vm_pager_destroy(vm_pager_t* pager)
{
    if (!pager)
        return;

    // If the pager has any resources to clean up, do it here (e.g. close file handles for vnode
    // pager)

    kfree(pager);
}

vm_pager_ops_t anon_pager_ops = {.get_page    = anon_pager_get_page,
                                 .alloc_page  = anon_pager_alloc_page,
                                 .put_page    = anon_pager_put_page,
                                 .has_page    = anon_pager_has_page,
                                 .lookup_page = anon_pager_lookup_page,
                                 .destroy     = anon_pager_destroy};

int anon_pager_get_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page)
{
    list_node_t* node;
    list_for_each(node, &obj->pages)
    {
        vm_page_t* p = list_node_to_page(node);
        if (p->offset != offset)
            continue;

        *page = p;
        return 0; // Page found
    }

    return -ENOENT; // Page not found
}

int anon_pager_alloc_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page)
{
    // Page doesn't exist, so we need to allocate a new one
    vm_page_t* new_page = kmalloc(sizeof(vm_page_t));
    if (!new_page)
        return -ENOMEM;

    new_page->offset    = offset;
    new_page->phys_addr = vm_phys_alloc_page();
    new_page->state     = 0;

    list_push_head(&obj->pages, &new_page->node);

    *page = new_page;

    return 0; // Success
}

int anon_pager_put_page(vm_object_t* obj, vm_page_t* page)
{
    // For an anonymous pager, we don't need to do anything special when putting a page
    // since the page is already part of the object and will be freed when the object is destroyed
    return 0;
}

bool anon_pager_has_page(vm_object_t* obj, vm_ooffset_t offset)
{
    list_node_t* node;
    list_for_each(node, &obj->pages)
    {
        vm_page_t* p = list_node_to_page(node);
        if (p->offset == offset) {
            return true; // Page exists
        }
    }

    return false; // Page does not exist
}

int anon_pager_lookup_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page)
{
    list_node_t* node;
    list_for_each(node, &obj->pages)
    {
        vm_page_t* p = list_node_to_page(node);
        if (p->offset == offset) {
            *page = p;
            return 0; // Page found
        }
    }

    return -ENOENT; // Page not found
}

void anon_pager_destroy(vm_object_t* obj)
{
    list_node_t* node;
    while (obj->pages.head) {
        node            = obj->pages.head;
        vm_page_t* page = list_node_to_page(node);
        vm_phys_free_page(page->phys_addr);
        list_remove(node);
        kfree(page);
    }
}

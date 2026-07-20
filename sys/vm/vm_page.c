#include "vm_page.h"
#include "kmalloc.h"
#include "vm_object.h"
#include "vm_phys.h"
#include <kern/errno.h>
#include <kern/spinlock.h>
#include <radix.h>

/* Byte offset -> page index, consistent with vm_object.c's add/remove_page. */
static inline unsigned long vm_page_index(size_t offset)
{
    return (unsigned long)(offset >> 12);
}

vm_page_t* vm_page_lookup(vm_object_t* obj, size_t offset)
{
    if (!obj)
        return NULL;

    vm_page_t* page = NULL;

    WITH_SPINLOCK(obj->lock)
    page = (vm_page_t*)radix_tree_lookup(&obj->pages, vm_page_index(offset));
    END_WITH_SPINLOCK

    return page;
}

vm_page_t* vm_page_allocate(vm_object_t* obj, size_t offset)
{
    if (!obj)
        return ERR_PTR(-EINVAL);

    uintptr_t phys = vm_phys_alloc_page();
    if (!phys)
        return ERR_PTR(-ENOMEM);

    vm_page_t* page = kmalloc(sizeof(*page));
    if (!page) {
        vm_phys_free_page(phys);
        return ERR_PTR(-ENOMEM);
    }
    page->phys_addr = phys;
    page->offset    = offset;

    WITH_SPINLOCK(obj->lock)
    int res = radix_tree_insert(&obj->pages, vm_page_index(offset), page);

    if (res) {
        // Someone else already inserted a page at this offset (race), or
        // the tree failed to allocate an internal node. Either way, this
        // page isn't the one that ended up in the object -- free it and
        // let the caller re-lookup.
        vm_phys_free_page(phys);
        kfree(page);
        return ERR_PTR(res);
    }
    END_WITH_SPINLOCK

    return page;
}

void vm_page_free(vm_page_t* page)
{
    if (!page)
        return;
    vm_phys_free_page(page->phys_addr);
    kfree(page);
}

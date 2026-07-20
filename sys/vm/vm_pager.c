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

int dead_pager_get_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page)
{
    return -ENOSYS; // Not implemented
}

int dead_pager_put_page(vm_object_t* obj, vm_page_t* page)
{
    return -ENOSYS; // Not implemented
}

bool dead_pager_has_page(vm_object_t* obj, vm_ooffset_t offset)
{
    return false; // Dead pager never has any pages
}

void dead_pager_destroy(vm_object_t* obj)
{
    // Nothing to clean up for the dead pager
}

vm_pager_ops_t dead_pager_ops = {.get_page = dead_pager_get_page,
                                 .put_page = dead_pager_put_page,
                                 .has_page = dead_pager_has_page,
                                 .destroy  = dead_pager_destroy};

#include "vm_vnode_pager.h"
#include "vm_object.h"
#include "vm_page.h"

#include <fs/vnode.h>

#include <kern/errno.h>

int vnode_pager_get_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page)
{
    vnode_t* vnode = (vnode_t*)obj->pager_data;
    if (!vnode)
        return -ENOSYS; // Not implemented

    vm_page_t* new_page = vm_page_allocate(obj, offset);
    if (!new_page)
        return -ENOMEM;

    return -ENOSYS; // Not implemented
}

int vnode_pager_put_page(vm_object_t* obj, vm_page_t* page)
{
    return -ENOSYS; // Not implemented
}

bool vnode_pager_has_page(vm_object_t* obj, vm_ooffset_t offset)
{
    return false; // Dead pager never has any pages
}

void vnode_pager_destroy(vm_object_t* obj)
{
    vnode_t* vnode = (vnode_t*)obj->pager_data;
    if (vnode) {
        vnode_dec_ref(vnode);

        obj->pager_data = NULL;
    }
}

vm_pager_ops_t vnode_pager_ops = {.get_page = vnode_pager_get_page,
                                  .put_page = vnode_pager_put_page,
                                  .has_page = vnode_pager_has_page,
                                  .destroy  = vnode_pager_destroy};

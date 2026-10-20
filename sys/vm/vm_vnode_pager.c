#include "vm_vnode_pager.h"
#include "vm_object.h"
#include "vm_page.h"
#include "vm_temp.h"
#include <fs/vnode.h>
#include <kern/errno.h>
#include <kern/terminal.h>

int vnode_pager_get_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t* page)
{
    vnode_t* vnode = (vnode_t*)obj->pager->data;
    if (!vnode)
        return -EINVAL;

    VM_TEMP_GUARD void* virt = vm_temp_map_page(page->phys_addr, VM_PROT_READ | VM_PROT_WRITE);

    // Read the page from the vnode into the vm_page_t's physical address
    int res = vnode->v_ops->read(vnode, virt, PAGE_SIZE, (size_t)offset);
    if (res < 0)
        return res;

    return 0;
}

// TODO: Check for read-only filesystems and handle errors accordingly
int vnode_pager_put_page(vm_object_t* obj, vm_page_t* page)
{
    vnode_t* vnode = (vnode_t*)obj->pager->data;
    if (!vnode)
        return -EINVAL;

    // Write the page from the vm_page_t's physical address to the vnode
    int res = vnode->v_ops->write(vnode, (void*)page->phys_addr, page->offset, PAGE_SIZE);
    if (res < 0)
        return res;

    return 0;
}

bool vnode_pager_has_page(vm_object_t* obj, vm_ooffset_t offset)
{
    vnode_t* vnode = (vnode_t*)obj->pager->data;
    if (!vnode)
        return false;

    // Check if the offset is within the vnode's size
    return true; // For simplicity, assume the vnode has all pages. In a real implementation, check
                 // the vnode's size and whether the page is present.
}

void vnode_pager_destroy(vm_object_t* obj)
{
    vnode_t* vnode = (vnode_t*)obj->pager->data;
    if (vnode) {
        vnode_dec_ref(vnode);

        obj->pager->data = NULL;
    }
}

vm_pager_ops_t vnode_pager_ops = {.get_page = vnode_pager_get_page,
                                  .put_page = vnode_pager_put_page,
                                  .has_page = vnode_pager_has_page,
                                  .destroy  = vnode_pager_destroy};

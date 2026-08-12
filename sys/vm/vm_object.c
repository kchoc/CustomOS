#include "vm_object.h"
#include "kmalloc.h"
#include "vm_page.h"
#include "vm_pager.h"
#include "vm_vnode_pager.h"

#include <fs/vfs.h>

#include <kern/errno.h>

#include <list.h>

/* radix_tree_destroy() callback: frees each remaining vm_page_t in the tree */
static void vm_object_free_page_cb(void* page)
{
    vm_page_free((vm_page_t*)page);
}

void vm_object_inc_ref(vm_object_t* obj)
{
    __sync_fetch_and_add(&obj->ref_count, 1);
}

void vm_object_dec_ref(vm_object_t* obj)
{
    if (__sync_sub_and_fetch(&obj->ref_count, 1) == 0) {
        // Free the object and its pages if this was the last reference

        WITH_SPINLOCK(obj->lock)
        {

            if (obj->pager) {
                if (obj->pager->ops && obj->pager->ops->destroy) {
                    obj->pager->ops->destroy(obj);
                }
                kfree(obj->pager);
                obj->pager = NULL;
            }

            radix_tree_destroy(&obj->pages, vm_object_free_page_cb);

            if (obj->shadow)
                vm_object_dec_ref(obj->shadow);
        }

        kfree(obj);
    }
}

vm_object_t* vm_object_create_anon(void)
{
    vm_object_t* new_obj = kmalloc(sizeof(vm_object_t));
    if (!new_obj)
        return ERR_PTR(-ENOMEM);

    new_obj->type          = VM_OBJECT_ANON;
    new_obj->ref_count     = 1;
    new_obj->shadow        = NULL;
    new_obj->shadow_offset = 0;
    new_obj->lock          = SPINLOCK_INITIALIZER;

    new_obj->pager = vm_pager_create(&dead_pager_ops, NULL);
    if (IS_ERR(new_obj->pager)) {
        kfree(new_obj);
        return ERR_PTR(-ENOMEM);
    }
    radix_tree_init(&new_obj->pages, VM_RADIX_CHUNK_BITS, VM_RADIX_HEIGHT);

    return new_obj;
}

vm_object_t* vm_object_create_shadow(vm_object_t* shadow, vm_ooffset_t offset)
{
    vm_object_t* new_obj = kmalloc(sizeof(vm_object_t));
    if (!new_obj)
        return ERR_PTR(-ENOMEM);

    new_obj->type          = VM_OBJECT_SHADOW;
    new_obj->ref_count     = 1;
    new_obj->shadow        = shadow;
    new_obj->shadow_offset = offset;
    new_obj->lock          = SPINLOCK_INITIALIZER;

    new_obj->pager = vm_pager_create(&dead_pager_ops, NULL);
    radix_tree_init(&new_obj->pages, 3, 4); // Example: 8 entries per node, 4 levels

    if (shadow) {
        vm_object_inc_ref(shadow);
    }

    return new_obj;
}

vm_object_t* vm_object_create_vnode(vnode_t* vnode)
{
    vm_object_t* obj = kmalloc(sizeof(vm_object_t));
    if (!obj) {
        return ERR_PTR(-ENOMEM);
    }

    vm_pager_t* pager = vm_pager_create(&vnode_pager_ops, vnode);
    if (IS_ERR(pager)) {
        kfree(obj);
        return ERR_PTR(-ENOMEM);
    }

    obj->pager_data    = vnode;
    obj->type          = VM_OBJECT_VNODE;
    obj->ref_count     = 1;
    obj->shadow        = NULL;
    obj->shadow_offset = 0;
    obj->lock          = SPINLOCK_INITIALIZER;

    obj->pager = pager;
    radix_tree_init(&obj->pages, 3, 4); // Example: 8 entries per node, 4 levels

    return obj;
}

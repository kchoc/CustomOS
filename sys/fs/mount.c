#include "mount.h"
#include "vnode_cache.h"
#include "vnode.h"

#include <vm/kmalloc.h>

#include <kern/spinlock.h>
#include <kern/errno.h>
#include <kern/panic.h>

mount_t *mount_table[MAX_MOUNTS];
int mount_count = 0;
mount_t *root_mnt = NULL;

void mount_inc_ref(mount_t* mnt) {
    __sync_fetch_and_add(&mnt->ref_count, 1);
}

void mount_dec_ref(mount_t* mnt) {
    if (__sync_fetch_and_sub(&mnt->ref_count, 1) == 1) {
        mount_destroy(mnt);
    }
}

int mount_create(vnode_t* mnt_point, vnode_t* mnt_dev_vnode, mount_flags_t flags, mount_t** result) {
    if (!result) return -EINVAL;
    if (mnt_dev_vnode && mnt_dev_vnode->v_type != VNODE_TYPE_BLOCK_DEVICE) return -ENOTBLK;

    if (__sync_fetch_and_add(&mount_count, 1) >= MAX_MOUNTS) {
        __sync_fetch_and_sub(&mount_count, 1);
        return -EMFILE; // Too many mounts
    }

    mount_t* mnt = kmalloc(sizeof(mount_t));
    if (!mnt) return -ENOMEM;

    if (mnt_point) vnode_inc_ref(mnt_point);
    if (mnt_point) vnode_inc_ref(mnt_dev_vnode);
    mnt->mnt_point = mnt_point;
    mnt->mnt_dev_vnode = mnt_dev_vnode;
    mnt->private = NULL;
    mnt->mnt_flags = flags;
    mnt->ref_count = 1;
    mnt->mnt_parent = NULL;
    
    mnt->vnode_count = 0;
    mnt->vnode_list = NULL;
    mnt->vnode_list_lock = 0;
    mnt->lazy_vnode_count = 0;
    mnt->lazy_vnode_list = NULL;
    mnt->lazy_vnode_list_lock = 0; 

    mount_table[mount_count - 1] = mnt;
    *result = mnt;

    return 0; // Success
}

int mount_destroy(mount_t* mnt) {
    if (!mnt) return -EINVAL;
  
    if (mnt->vnode_count > 0) return -EBUSY; // Cannot destroy mount with active vnodes
    
    WITH_SPINLOCK(mnt->lazy_vnode_list_lock)
    vnode_t* lazy_vnode = mnt->lazy_vnode_list;
    while (lazy_vnode) {
        vnode_t* next = lazy_vnode->mnt_next;
        if (vnode_cache_remove(lazy_vnode)) PANIC("Failed to remove lazy vnode from cache during mount destruction\n");
        kfree(lazy_vnode);
        lazy_vnode = next;
    }
    mnt->lazy_vnode_list = NULL;
    mnt->lazy_vnode_count = 0;
    END_WITH_SPINLOCK

    if (mnt->mnt_ops && mnt->mnt_ops->unmount) {
        int res = mnt->mnt_ops->unmount(mnt);
        if (res) return res; // Unmount failed
    }

    vnode_dec_ref(mnt->mnt_point);
    vnode_dec_ref(mnt->mnt_dev_vnode);

    kfree(mnt->mnt_point);
    kfree(mnt);

    return 0; // Success
}

mount_t *lookup_mnt_by_vnode(vnode_t* vnode) {
    if (!vnode) return NULL;
    for (int i = 0; i < mount_count; i++) {
        if (mount_table[i] && mount_table[i]->mnt_point == vnode)
            return mount_table[i];
    }
    return NULL;
}


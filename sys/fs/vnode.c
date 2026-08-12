#include "vnode.h"
#include "fs/types.h"
#include "kern/spinlock.h"
#include "mount.h"
#include "vnode_cache.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/terminal.h>

void vnode_inc_ref(vnode_t* vnode)
{
    __sync_fetch_and_add(&vnode->v_refcount, 1);
}

void vnode_dec_ref(vnode_t* vnode)
{
    if (__sync_fetch_and_sub(&vnode->v_refcount, 1) == 1) {
        vnode_inactive(vnode); // Mark the vnode as inactive before reclaiming
        vnode_reclaim(vnode);  // Reclaim the vnode when ref count reaches zero
    }
}

int vnode_get(mount_t* mount, uint64_t file_id, vnode_t** result)
{
    return vnode_cache_lookup(mount, file_id, result);
}

void vnode_inactive(vnode_t* vnode)
{
    vnode->v_ops->inactive(vnode);
    WITH_SPINLOCK(vnode->v_mount->vnode_list_lock)
    vnode_t** current = &vnode->v_mount->vnode_list;
    while (*current) {
        if (*current == vnode) {
            *current = vnode->mnt_next; // Remove from mount's vnode list
            vnode->v_mount->vnode_count--;
            goto found; // Exit loop after removing
        }
        current = &(*current)->mnt_next;
    }

    PANIC("Failed to remove vnode from mount's vnode list during inactivation\n");

found:
    WITH_SPINLOCK(vnode->v_mount->lazy_vnode_list_lock)

    vnode->mnt_next                 = vnode->v_mount->lazy_vnode_list;
    vnode->v_mount->lazy_vnode_list = vnode;
    vnode->v_mount->lazy_vnode_count++;

    END_WITH_SPINLOCK
    END_WITH_SPINLOCK
}

void vnode_reclaim(vnode_t* vnode)
{
    vnode->v_ops->reclaim(vnode);
    WITH_SPINLOCK(vnode->v_mount->lazy_vnode_list_lock)
    while (vnode->v_mount->lazy_vnode_list) {
        if (vnode->v_mount->lazy_vnode_list == vnode) {
            vnode->v_mount->lazy_vnode_list =
                vnode->v_mount->lazy_vnode_list->mnt_next; // Remove from lazy vnode list
            vnode->v_mount->lazy_vnode_count--;
            vnode_cache_remove(vnode); // Remove from vnode cache
            kfree(vnode);              // Free the vnode memory
            return;                    // Exit after removing
        }
        vnode->v_mount->lazy_vnode_list = vnode->v_mount->lazy_vnode_list->mnt_next;
    }
    END_WITH_SPINLOCK

    PANIC("Failed to remove vnode from mount's lazy vnode list during reclamation\n");
}

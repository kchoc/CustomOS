#include "vnode.h"
#include "fs/types.h"
#include "kern/spinlock.h"
#include "mount.h"
#include "vnode_cache.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/terminal.h>

#define MAX_VNODE_CACHE_SIZE 1024

void vnode_inc_ref(vnode_t* vnode)
{
    __sync_fetch_and_add(&vnode->ref_count, 1);
}

void vnode_dec_ref(vnode_t* vnode)
{
    if (__sync_fetch_and_sub(&vnode->ref_count, 1) == 1)
    {
        vnode_inactive(vnode); // Mark the vnode as inactive before reclaiming
        vnode_reclaim(vnode);  // Reclaim the vnode when ref count reaches zero
    }
}

int vnode_get(mount_t* mount, uint64_t file_id, vnode_t** result)
{
    int res = vnode_cache_lookup(mount, file_id, result);
    if (res != 0 && res != -ENOENT)
        return res; // Return error if lookup failed for reasons other than not found
    if (res == 0)
    {
        vnode_inc_ref(*result); // Increment ref count for the caller
        return 0;               // Success
    }

    vnode_t* vnode = kmalloc(sizeof(vnode_t)); // Allocate memory for the result pointer
    if (!vnode)
        return -ENOMEM;

    vnode->v_mount = mount;
    vnode->v_mounthere = NULL;
    vnode->v_ops = NULL;  // To be set by the filesystem-specific code after retrieval
    vnode->v_data = NULL; // To be set by the filesystem-specific code after retrieval
    vnode->v_type = 0;    // To be set by the filesystem-specific code after
    vnode->file_id = file_id;
    vnode->ref_count = 1; // Start with a reference count of 1 for the callee

    vnode_cache_insert(vnode); // Insert into the vnode cache for quick lookup

    WITH_SPINLOCK(mount->vnode_list_lock)

    mount->vnode_count++;
    vnode->mnt_next = mount->vnode_list;
    mount->vnode_list = vnode;

    END_WITH_SPINLOCK

    *result = vnode;
    return 0; // Success
}

void vnode_inactive(vnode_t* vnode)
{
    vnode->v_ops->inactive(vnode);
    WITH_SPINLOCK(vnode->v_mount->vnode_list_lock)
    vnode_t** current = &vnode->v_mount->vnode_list;
    while (*current)
    {
        if (*current == vnode)
        {
            *current = vnode->mnt_next; // Remove from mount's vnode list
            vnode->v_mount->vnode_count--;
            goto found; // Exit loop after removing
        }
        current = &(*current)->mnt_next;
    }
    END_WITH_SPINLOCK

    PANIC("Failed to remove vnode from mount's vnode list during inactivation\n");

found:
    WITH_SPINLOCK(vnode->v_mount->lazy_vnode_list_lock)

    vnode->mnt_next = vnode->v_mount->lazy_vnode_list;
    vnode->v_mount->lazy_vnode_list = vnode;
    vnode->v_mount->lazy_vnode_count++;

    END_WITH_SPINLOCK
}

void vnode_reclaim(vnode_t* vnode)
{
    vnode->v_ops->reclaim(vnode);
    WITH_SPINLOCK(vnode->v_mount->lazy_vnode_list_lock)
    while (vnode->v_mount->lazy_vnode_list)
    {
        if (vnode->v_mount->lazy_vnode_list == vnode)
        {
            vnode->v_mount->lazy_vnode_list =
                vnode->v_mount->lazy_vnode_list->mnt_next; // Remove from lazy vnode list
            vnode_cache_remove(vnode);                     // Remove from vnode cache
            kfree(vnode);                                  // Free the vnode memory
            return;                                        // Exit after removing
        }
        vnode->v_mount->lazy_vnode_list = vnode->v_mount->lazy_vnode_list->mnt_next;
    }
    vnode->v_mount->lazy_vnode_count--;
    END_WITH_SPINLOCK

    PANIC("Failed to remove vnode from mount's lazy vnode list during reclamation\n");
}

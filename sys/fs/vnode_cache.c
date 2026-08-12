#include "vnode_cache.h"
#include "mount.h"
#include "vnode.h"

#include <kern/errno.h>
#include <kern/spinlock.h>
#include <kern/terminal.h>

#include <vm/kmalloc.h>

#include <string.h>

vnode_cache_bucket_t node_cache[MAX_VNODE_CACHE_SIZE];

int vnode_cache_init(void)
{
    memset(node_cache, 0, sizeof(node_cache));
    return 0;
}

int vnode_cache_remove(vnode_t* vnode)
{
    if (!vnode)
        return -EINVAL;

    uint64_t key   = (uint32_t)vnode->v_mount ^ vnode->file_id;
    int      index = key % MAX_VNODE_CACHE_SIZE;

    WITH_SPINLOCK(node_cache[index].lock);

    vnode_t* current = node_cache[index].head;
    vnode_t* prev    = NULL;
    while (current) {
        if (current == vnode) {
            if (prev) {
                prev->hash_next = current->hash_next;
            }
            else {
                node_cache[index].head = current->hash_next;
            }
            return 0; // Success
        }
        prev    = current;
        current = current->hash_next;
    }

    END_WITH_SPINLOCK

    return -ENOENT; // Not found
}

int vnode_cache_lookup(mount_t* mnt, uint64_t file_id, vnode_t** result)
{
    if (!mnt || !result)
        return -EINVAL;

    uint64_t key   = (uint32_t)mnt ^ file_id;
    int      index = key % MAX_VNODE_CACHE_SIZE;

    WITH_SPINLOCK(node_cache[index].lock);

    vnode_t* current = node_cache[index].head;
    while (current) {
        if (current->v_mount == mnt && current->file_id == file_id) {
            vnode_inc_ref(current);
            *result = current;
            return 0; // Success
        }
        current = current->hash_next;
    }

    // Not found so insert a new vnode into the cache
    vnode_t* new_vnode = kmalloc(sizeof(vnode_t));
    if (!new_vnode)
        return -ENOMEM;

    new_vnode->v_mount     = mnt;
    new_vnode->v_mounthere = NULL;

    new_vnode->v_ops  = NULL;
    new_vnode->v_data = NULL;
    new_vnode->v_type = 0;

    new_vnode->file_id    = file_id;
    new_vnode->v_refcount = 1;

    new_vnode->hash_next   = node_cache[index].head;
    node_cache[index].head = new_vnode;

    WITH_SPINLOCK(mnt->vnode_list_lock)

    new_vnode->mnt_next = mnt->vnode_list;
    mnt->vnode_list     = new_vnode;
    mnt->vnode_count++;

    END_WITH_SPINLOCK;

    *result = new_vnode;

    END_WITH_SPINLOCK;

    return 1; // Indicate that a new vnode was created and added to the cache
}

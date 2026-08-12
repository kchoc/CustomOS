#include "iname.h"
#include "mount.h"
#include "vfs.h"
#include "vnode.h"
#include "vnode_cache.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

int iname_lookup(const char* name, vnode_t* dir, vnode_t** result)
{
    if (!name || !result)
        return -EINVAL;
    if (dir && dir->v_type != VNODE_TYPE_DIRECTORY)
        return -ENOTDIR;

    char* name_copy = strdup(name);
    if (!name_copy)
        return -ENOMEM;

    char* token = strtok(name_copy, "/");

    vnode_t* current = dir ? dir : root_vnode; // Start from provided dir or root
    vnode_inc_ref(current);                    // Increment ref count for the starting vnode

    vnode_t* next;
    while (token) {
        if (current->v_mounthere) {
            // TODO: I need to handle getting the v_mount properly here as there is an unlikely case
            // that the mount could be unmounted while we are traversing it, but I will handle that
            // later
            current->v_mounthere->mnt_ops->get_root(
                current->v_mounthere, &next); // Switch to the mounted filesystem's root vnode
            vnode_dec_ref(current);           // Decrement ref count for the current vnode
            current = next;                   // Move to the mounted filesystem's root vnode
        }
        int res = current->v_ops->lookup(current, token, &next);
        if (res) {
            printf("iname_lookup: Failed to lookup '%s' in vnode with type %d, error %d\n", token,
                   current->v_type, res);
            delay(2000);
            kfree(name_copy);
            return res; // Lookup failed
        }
        vnode_dec_ref(current); // Decrement ref count for the current vnode
        current = next;         // Move to the next vnode
        token   = strtok(NULL, "/");
    }

    if (current->v_mounthere) {
        current->v_mounthere->mnt_ops->get_root(
            current->v_mounthere, &next); // Switch to the mounted filesystem's root vnode
        vnode_dec_ref(current);           // Decrement ref count for the current vnode
        current = next;                   // Move to the mounted filesystem's root vnode
    }

    *result = current; // Set the result to the final vnode found
    kfree(name_copy);
    return 0; // Success
}

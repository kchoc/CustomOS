#include "mount.h"
#include "vnode.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/terminal.h>

mount_ops_t devfs_mount_ops = {
    .mount    = devfs_mount,
    .unmount  = devfs_unmount,
    .sync     = DISALLOWED_OP,
    .get_root = devfs_get_root,
};

int devfs_mount(mount_t* mnt, const char* options)
{
    // DEVFS root vnode needs to be created and initialized here. The root vnode will represent the
    // /dev directory.
    vnode_t* root_vnode;
    int      res = devfs_get_root(mnt, &root_vnode);
    if (res)
        return res;

    return 0;
}

int devfs_unmount(mount_t* mnt)
{
    return -ENOTSUP; // Unmounting devfs is not supported in this implementation
}

int devfs_get_root(mount_t* mnt, vnode_t** vnode)
{
    if (!mnt || !vnode)
        return -EINVAL;

    int res = vnode_get(mnt, 0, vnode);
    if (IS_ERR(res))
        return res;

    if (res == 0)
        return 0;

    (*vnode)->v_type = VNODE_TYPE_DIRECTORY;
    (*vnode)->v_ops  = &devfs_vnode_ops;

    devfs_vnode_data_t* root_data = kmalloc(sizeof(devfs_vnode_data_t));
    if (!root_data) {
        vnode_dec_ref(*vnode); // Decrement ref count on failure to allocate devfs data
        return -ENOMEM;
    }

    root_data->lock = SPINLOCK_INITIALIZER;
    list_init(&root_data->device_block, 0);

    (*vnode)->v_data = root_data; // Link the root vnode to its devfs data

    return 0;
}

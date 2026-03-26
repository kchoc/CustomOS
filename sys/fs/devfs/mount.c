#include "mount.h"
#include "vnode.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/terminal.h>

mount_ops_t devfs_mount_ops = {
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .get_root = devfs_get_root,
    .sync = DISALLOWED_OP,
};

int devfs_mount(mount_t* mnt, const char* options)
{
    // Create the root vnode for devfs if it doesn't already exist
    devfs_vnode_data_t* root_data = kmalloc(sizeof(devfs_vnode_data_t));
    if (!root_data)
        return -ENOMEM;

    root_data->lock = 0; // Initialize the spinlock
    list_init(&root_data->device_block, 0);

    int res = vnode_get(mnt, 0, &mnt->mnt_point);
    if (res)
    {
        kfree(root_data);
        return res;
    }

    mnt->mnt_point->v_ops = &devfs_vnode_ops;
    mnt->mnt_point->v_data = root_data;
    mnt->mnt_point->v_type = VNODE_TYPE_DIRECTORY; // The root of devfs is a directory

    return 0; // Success
}

int devfs_unmount(mount_t* mnt)
{
    // No special unmounting logic needed for devfs since it's pseudo and doesn't rely on a device
    return 0; // Success
}

int devfs_get_root(mount_t* mnt, vnode_t** vnode)
{
    if (!mnt || !vnode)
        return -EINVAL;

    if (mnt->mnt_point)
    {
        *vnode = mnt->mnt_point;
        vnode_inc_ref(*vnode); // Increment ref count for the caller
        return 0;              // Success
    }

    PANIC("devfs_get_root: mount point is NULL, devfs should always have a root vnode after "
          "successful mount\n");
    return -ENOENT; // Root vnode not found (shouldn't happen if mount was successful)
}

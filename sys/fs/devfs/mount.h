#ifndef DEVFS_MOUNT_H
#define DEVFS_MOUNT_H

#include <fs/mount.h>

extern mount_ops_t devfs_mount_ops;

int devfs_mount(mount_t* mnt, const char* options);
int devfs_unmount(mount_t* mnt);
int devfs_get_root(mount_t* mnt, vnode_t** vnode);

#endif // DEVFS_MOUNT_H


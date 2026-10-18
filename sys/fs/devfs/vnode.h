#ifndef FS_DEVFS_VNODE_H
#define FS_DEVFS_VNODE_H

#include <fs/vnode.h>

#include <sys/device.h>

#include <kern/spinlock.h>

#include <list.h>

#define DEVICES_PER_BLOCK 8

typedef struct devfs_device_block {
    list_node_t node;                       // For linking in the device_block list
    vnode_t*    devices[DEVICES_PER_BLOCK]; // Array of pointers to device vnodes
} devfs_device_block_t;

typedef struct devfs_vnode_data {
    spinlock_t lock;
    list_t     device_block;
} devfs_vnode_data_t;

extern mount_t* dev_mount;

CREATE_VNODE_OPS(devfs);

int devfs_init();

#endif // FS_DEVFS_VNODE_H

#ifndef FS_DEVFS_VNODE_H
#define FS_DEVFS_VNODE_H

#include <fs/vnode.h>

#include <sys/device.h>

#include <kern/spinlock.h>

#include <list.h>

#define DEVICES_PER_BLOCK 8

typedef struct devfs_device_block {
    list_node_t node;                       // For linking in the device_block list
    vnode_t*    devices[DEVICES_PER_BLOCK]; // Array of 8 devices per block to reduce overhead of
                                            // many small allocations
} devfs_device_block_t;

typedef struct devfs_vnode_data {
    spinlock_t lock;         // Lock to protect access to the device list
    list_t     device_block; // List of devfs_device entries representing devices in this directory
} devfs_vnode_data_t;

extern mount_t* dev_mount; // The devfs mount structure

CREATE_VNODE_OPS(devfs)

int devfs_init();

#endif // FS_DEVFS_VNODE_H

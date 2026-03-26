#ifndef FS_DEVFS_VNODE_H
#define FS_DEVFS_VNODE_H

#include <fs/vnode.h>

#include <sys/device.h>

#include <kern/spinlock.h>

#include <list.h>

#define DEVICES_PER_BLOCK 8

typedef struct devfs_device {
    char          name[MAX_DEVICE_NAME_LEN]; // Name of the device (e.g., "sda", "tty0")
    device_type_t type;                      // Type of the device (block or char)
    void*
        device; // Pointer to the actual device structure (e.g., block_device_t* or char_device_t*)
    vnode_t* vnode; // Vnode cache reference for this device, can be NULL if not currently cached
} devfs_device_t;

typedef struct devfs_device_block {
    list_node_t    node;                       // For linking in the device_block list
    devfs_device_t devices[DEVICES_PER_BLOCK]; // Array of 8 devices per block to reduce overhead of
                                               // many small allocations
} devfs_device_block_t;

typedef struct devfs_vnode_data {
    spinlock_t lock;         // Lock to protect access to the device list
    list_t     device_block; // List of devfs_device entries representing devices in this directory
} devfs_vnode_data_t;

extern vnode_ops_t devfs_vnode_ops;
extern mount_t*    dev_mount; // The devfs mount structure

int devfs_init();

#endif // FS_DEVFS_VNODE_H

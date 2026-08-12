#include "vnode.h"
#include "file.h"
#include "mount.h"

#include <fs/mount.h>
#include <fs/vfs.h>
#include <fs/vnode.h>

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

#define MAX_DEVICE_NAME_LEN 32

mount_t* dev_mount = NULL;

int devfs_init()
{
    int res;

    res = mount_create(NULL, NULL, MOUNT_NONE, &dev_mount);
    if (res)
        return res;

    dev_mount->mnt_ops = &devfs_mount_ops;

    return dev_mount->mnt_ops->mount(dev_mount, NULL);
}

int devfs_vnode_lookup(vnode_t* dir, const char* name, vnode_t** result)
{
    if (!dir || !name || !result)
        return -EINVAL;

    WITH_SPINLOCK(((devfs_vnode_data_t*)dir->v_data)->lock)

    list_node_t* node;
    list_for_each(node, &((devfs_vnode_data_t*)dir->v_data)->device_block)
    {
        devfs_device_block_t* block = (devfs_device_block_t*)node;
        for (int i = 0; i < 8; i++) {
            vnode_t* dev_vnode = block->devices[i];
            if (!dev_vnode)
                continue; // Skip empty slots

            device_t* dev = (device_t*)dev_vnode->v_data;

            if (strncmp(dev->name, name, MAX_DEVICE_NAME_LEN) == 0) {
                *result = dev_vnode;
                vnode_inc_ref(dev_vnode); // Increment ref count for the returned vnode
                return 0;                 // Found the device
            }
        }
    }

    END_WITH_SPINLOCK;

    return -ENOENT; // Not found
}

/* Creats a new device vnode under the specified directory vnode.
 * The caller is responsible for setting the actual device pointer.
 * For faster access the vnode data is temporarily set to the devfs_device_t structure, but this
 * should be updated by the caller to point to the actual device structure (e.g., block_device_t or
 * char_device_t) after creation. */
int devfs_vnode_create(vnode_t* dir, const char* name, enum vnode_type type, vnode_t** result)
{
    if (!dir || !name || !result)
        return -EINVAL;
    if (type != VNODE_TYPE_BLOCK_DEVICE && type != VNODE_TYPE_CHAR_DEVICE)
        return -EINVAL;
    if (strlen(name) >= MAX_DEVICE_NAME_LEN)
        return -EINVAL; // Name too long for our fixed-size name field

    devfs_vnode_data_t* dir_data = (devfs_vnode_data_t*)dir->v_data;

    WITH_SPINLOCK(dir_data->lock)

    // Find an existing block with space for a new device
    devfs_device_block_t* block;
    list_node_t*          node;
    vnode_t*              dev_vnode;
    size_t                block_index = 0;
    size_t                index       = 0;
    list_for_each(node, &dir_data->device_block)
    {
        block = (devfs_device_block_t*)node;
        for (index = 0; index < DEVICES_PER_BLOCK; index++) {
            vnode_t* dev_vnode = block->devices[index];

            if (!dev_vnode)
                goto found_slot;
        }
        block_index++;
    }

    // No existing block has space, create a new block
    block = kmalloc(sizeof(devfs_device_block_t));
    if (!block)
        return -ENOMEM;

    memset(block, 0, sizeof(devfs_device_block_t)); // Clear the block
    list_push_tail(&dir_data->device_block, &block->node);

    block_index++;
    index = 0;

    int res;
found_slot:
    res = vnode_get(dev_mount, block_index * DEVICES_PER_BLOCK + index + 1,
                    &dev_vnode); // Unique vnode ID based on block and index
    if (IS_ERR(res)) {
        return res;
    }

    block->devices[index] = dev_vnode; // Store the new device vnode in the block

    *result = dev_vnode;
    if (res == 0)
        return 0; // Vnode already exists, return it

    // Initialize the new device vnode
    dev_vnode->v_type = type;
    dev_vnode->v_ops  = &devfs_vnode_ops;

    END_WITH_SPINLOCK

    return 0;
}

int devfs_vnode_unlink(vnode_t* dir, const char* name)
{
    if (!dir || !name)
        return -EINVAL;

    devfs_vnode_data_t* dir_data = (devfs_vnode_data_t*)dir->v_data;

    WITH_SPINLOCK(dir_data->lock)

    list_node_t* node;
    list_for_each(node, &dir_data->device_block)
    {
        devfs_device_block_t* block = (devfs_device_block_t*)node;
        for (int i = 0; i < DEVICES_PER_BLOCK; i++) {
            vnode_t* dev_vnode = block->devices[i];
            if (!dev_vnode)
                continue; // Skip empty slots

            device_t* dev = (device_t*)dev_vnode->v_data;
            if (!dev)
                continue; // Skip if device data is NULL

            if (strncmp(dev->name, name, MAX_DEVICE_NAME_LEN) == 0) {
                // Found the device to unlink
                block->devices[i] = NULL; // Remove the device from the block
                vnode_dec_ref(dev_vnode); // Decrement the reference count of the vnode
                return 0;
            }
        }
    }

    END_WITH_SPINLOCK;

    return -ENOENT; // Not found
}

int devfs_vnode_readdir(vnode_t* dir, void* buf, size_t size, size_t offset)
{
    if (!dir || !buf)
        return -EINVAL;

    size_t bytes_written  = 0;
    size_t current_offset = 0;

    WITH_SPINLOCK(((devfs_vnode_data_t*)dir->v_data)->lock)

    list_node_t* node;
    list_for_each(node, &((devfs_vnode_data_t*)dir->v_data)->device_block)
    {
        devfs_device_block_t* block = (devfs_device_block_t*)node;
        for (int i = 0; i < DEVICES_PER_BLOCK; i++) {
            vnode_t* dev_vnode = block->devices[i];
            if (!dev_vnode)
                continue; // Skip empty slots

            device_t* dev = (device_t*)dev_vnode->v_data;
            if (!dev)
                continue; // Skip if device data is NULL

            if (dev->name[0] != '\0') {
                size_t name_len = strnlen(dev->name, MAX_DEVICE_NAME_LEN);
                if (current_offset >= offset && bytes_written + name_len + 1 <= size) {
                    memcpy((char*)buf + bytes_written, dev->name, name_len);
                    bytes_written += name_len;
                    ((char*)buf)[bytes_written] = '\0'; // Null terminator for the entry
                    bytes_written++;
                }
                current_offset++;
            }
        }
    }

    END_WITH_SPINLOCK

    return bytes_written; // Return the total bytes written to the buffer
}

int devfs_vnode_open(vnode_t* vnode, file_t* file)
{
    if (!vnode)
        return -EINVAL;

    device_t* dev = (device_t*)vnode->v_data;
    if (!dev)
        return -EINVAL;

    if (!dev->ops || !dev->ops->open)
        return -ENODEV;

    file->f_ops = &devfs_file_ops; // Set the file operations for this file to the devfs file ops

    return dev->ops->open(dev);
}

int devfs_vnode_close(vnode_t* vnode)
{
    if (!vnode)
        return -EINVAL;

    device_t* dev = (device_t*)vnode->v_data;
    if (!dev)
        return -EINVAL;

    if (!dev->ops || !dev->ops->close)
        return -ENODEV;

    return dev->ops->close(dev);
}

int devfs_vnode_read(vnode_t* vnode, void* buf, size_t size, size_t offset)
{
    if (!vnode || !buf)
        return -EINVAL;

    device_t* dev = (device_t*)vnode->v_data;
    if (!dev)
        return -EINVAL;

    if (!dev->ops || !dev->ops->read)
        return -ENODEV;

    return dev->ops->read(dev, offset, size, buf);
}

int devfs_vnode_write(vnode_t* vnode, const void* buf, size_t size, size_t offset)
{
    if (!vnode || !buf)
        return -EINVAL;

    device_t* dev = (device_t*)vnode->v_data;
    if (!dev)
        return -EINVAL;

    if (!dev->ops || !dev->ops->write)
        return -ENODEV;

    return dev->ops->write(dev, offset, size, buf);
}

int devfs_vnode_ioctl(vnode_t* vnode, int cmd, void* arg)
{
    if (!vnode)
        return -EINVAL;

    device_t* dev = (device_t*)vnode->v_data;
    if (!dev)
        return -EINVAL;

    if (!dev->ops || !dev->ops->ioctl)
        return -ENODEV;

    return dev->ops->ioctl(dev, cmd, arg);
}

int devfs_vnode_reclaim(vnode_t* vnode)
{
    vnode->v_data = NULL; // Clear the data pointer, but do not free it since it may be shared with
                          // other vnodes (e.g., multiple references to the same device)

    return 0;
}

int devfs_vnode_inactive(vnode_t* vnode)
{
    // No special action needed for inactive vnodes in this implementation
    return 0;
}

vnode_ops_t devfs_vnode_ops = {.lookup   = devfs_vnode_lookup,
                               .create   = devfs_vnode_create,
                               .link     = DISALLOWED_OP,
                               .unlink   = devfs_vnode_unlink,
                               .rename   = DISALLOWED_OP,
                               .mkdir    = DISALLOWED_OP,
                               .rmdir    = DISALLOWED_OP,
                               .readdir  = devfs_vnode_readdir,
                               .open     = devfs_vnode_open,
                               .close    = devfs_vnode_close,
                               .read     = devfs_vnode_read,
                               .write    = devfs_vnode_write,
                               .ioctl    = devfs_vnode_ioctl,
                               .getattr  = DISALLOWED_OP,
                               .setattr  = DISALLOWED_OP,
                               .truncate = DISALLOWED_OP,
                               .access   = DISALLOWED_OP,
                               .symlink  = DISALLOWED_OP,
                               .readlink = DISALLOWED_OP,
                               .mknod    = DISALLOWED_OP,
                               .fsync    = DISALLOWED_OP,
                               .inactive = devfs_vnode_inactive,
                               .reclaim  = devfs_vnode_reclaim};

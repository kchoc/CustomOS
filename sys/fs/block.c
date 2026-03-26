#include "block.h"

#include <fs/vnode.h>

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

int block_read(device_t* bdev, uint64_t block_num, void** buffer, size_t block_size)
{
    if (!bdev || !buffer)
        return -EINVAL; // Invalid arguments

    *buffer = kmalloc(block_size);
    if (!*buffer)
        return -ENOMEM;

    int res =
        bdev->ops->read(bdev, block_num, block_size / 512,
                        *buffer); // Read the block from the device (assuming 512 bytes per sector)
    return res;
}

int block_write(device_t* bdev, uint64_t block_num, const void* buffer, size_t block_size)
{
    if (!bdev || !buffer)
        return -EINVAL; // Invalid arguments

    int res =
        bdev->ops->write(bdev, block_num, block_size / 512,
                         buffer); // Write the block to the device (assuming 512 bytes per sector)
    return res;
}

void block_release(void* buffer)
{
    if (buffer)
        kfree(buffer); // Free the allocated buffer
}

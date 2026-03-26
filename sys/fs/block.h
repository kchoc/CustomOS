#ifndef FS_BLOCK_H
#define FS_BLOCK_H

#include <fs/vnode.h>

#include <sys/device.h>

#include <stdint.h>

int block_read(device_t* bdev, uint64_t block_num, void** buffer, size_t block_size);
int block_write(device_t* bdev, uint64_t block_num, const void* buffer, size_t block_size);

void block_release(void* buffer);

#endif // FS_BLOCK_H

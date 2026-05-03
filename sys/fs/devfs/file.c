#include "file.h"

#include "vnode.h"

#include <kern/errno.h>

int devfs_file_read(file_t* file, void* buf, size_t size)
{
    if (!file || !buf)
        return -EINVAL;

    return devfs_vnode_read(file->f_vnode, buf, size, file->f_pos); 
}

int devfs_file_write(file_t* file, const void* buf, size_t size)
{
    if (!file || !buf)
        return -EINVAL;

    return devfs_vnode_write(file->f_vnode, buf, size, file->f_pos);
}

int devfs_file_seek(file_t* file, size_t offset, int whence)
{
    if (!file)
        return -EINVAL;

    // For simplicity, only support SEEK_SET for now since most devices don't really need
    // arbitrary seeking and it simplifies the implementation. We can add more seek modes in the
    // future if needed.
    if (whence != SEEK_SET)
        return -EINVAL;

    file->f_pos = offset;
    return 0; 
}

int devfs_file_close(file_t* file)
{
    if (!file)
        return -EINVAL;

    return devfs_vnode_close(file->f_vnode);
}

int devfs_file_ioctl(file_t* file, int request, void* arg)
{
    if (!file)
        return -EINVAL;

    return devfs_vnode_ioctl(file->f_vnode, request, arg); 
}

file_ops_t devfs_file_ops = {
    .read = devfs_file_read,
    .write = devfs_file_write,
    .seek = devfs_file_seek,
    .close = devfs_file_close,
    .ioctl = devfs_file_ioctl
};


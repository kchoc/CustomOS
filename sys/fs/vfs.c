#include "vfs.h"

#include "file.h"
#include "filesystem.h"
#include "iname.h"
#include "mount.h"
#include "vnode_cache.h"

#include "devfs/vnode.h"
#include "pseudo/vnode.h"
#include "sockfs.h"
#include "vfat/filesystem.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/pit.h>
#include <kern/terminal.h>

#include <inttypes.h>
#include <list.h>
#include <string.h>

vnode_t* root_vnode = NULL;

/*  =================
    VFS BLOCK DEVICES
    ================= */

int vfs_register_device(device_t* dev)
{
    int      res;
    vnode_t* dev_vnode;

    res = dev_mount->mnt_point->v_ops->create(dev_mount->mnt_point, dev->name,
                                              VNODE_TYPE_BLOCK_DEVICE, &dev_vnode);
    if (res)
        return res;

    devfs_device_t* devfs_data = (devfs_device_t*)dev_vnode->v_data;
    devfs_data->device         = dev; // Link the block device to the devfs vnode data
    dev_vnode->v_data          = dev;

    if (!root_vnode)
        vfs_mount_root(dev->name);

    return 0;
}

device_t* vfs_get_device(const char* name)
{
    vnode_t* dev_vnode;
    int      res = dev_mount->mnt_point->v_ops->lookup(dev_mount->mnt_point, name, &dev_vnode);
    if (res)
        return NULL;

    device_t* bdev = (device_t*)dev_vnode->v_data;
    if (!bdev)
        return NULL;

    return bdev;
}

void vfs_list_devices(void)
{
    char buf[256];

    printf("Block devices in /dev:\n");

    int bytes = dev_mount->mnt_point->v_ops->readdir(dev_mount->mnt_point, buf, sizeof(buf), 0);
    if (is_errno(bytes))
        PANIC("Failed to read /dev directory for block devices\n");

    size_t offset = 0;
    while (offset < (size_t)bytes) {
        char*  name     = buf + offset;
        size_t name_len = strlen(name);
        if (name_len == 0)
            break; // End of entries
        device_t* dev = vfs_get_device(name);
        if (dev) {
            printf(" - %s\n", dev->name);
        }

        offset += name_len + 1; // Move to the next entry
    }
}

/* ==================
   VFS INITIALIZATION
   ================== */

int vfs_init(void)
{
    int res;
    vnode_cache_init();

    register_filesystem(&vfat_fs_type);

    res = devfs_init();
    if (res)
        PANIC_RES("Failed to initialize devfs", res);

    res = mount_create(NULL, NULL, MOUNT_NONE, &root_mnt);
    if (res)
        PANIC_RES("Failed to create root mount", res);

    return 0; // Success
}

int vfs_mount_root(const char* device_name)
{
    if (!device_name)
        return -EINVAL;

    int res = dev_mount->mnt_point->v_ops->lookup(dev_mount->mnt_point, device_name,
                                                  &root_mnt->mnt_dev_vnode);
    if (res)
        return res;

    if (root_mnt->mnt_dev_vnode->v_type != VNODE_TYPE_BLOCK_DEVICE) {
        vnode_dec_ref(root_mnt->mnt_dev_vnode); // Decrement ref count since we won't use it
        root_mnt->mnt_dev_vnode = NULL;         // Clear the reference on failure
        return -ENOTBLK;
    }

    file_system_type_t* fs_type;
    res = get_filesystem_type("vfat", &fs_type);
    if (res)
        PANIC("Failed to find VFAT filesystem type\n");

    root_mnt->mnt_ops = fs_type->fs_ops;

    if (root_mnt->mnt_ops->mount(root_mnt, NULL))
        PANIC("Failed to mount root filesystem\n");

    res = root_mnt->mnt_ops->get_root(root_mnt, &root_vnode);
    if (res)
        PANIC("Failed to get root vnode from root mount\n");

    vnode_t* dev_root_vnode;
    res = root_vnode->v_ops->lookup(root_vnode, "dev", &dev_root_vnode);
    if (res)
        PANIC("Failed to lookup /dev in root filesystem\n");

    dev_root_vnode->v_mounthere = dev_mount; // Link the /dev vnode to the devfs mount so that we
                                             // can switch to it when traversing into /dev

    return 0; // Success
}

/*  ===================
    VFS FILE OPERATIONS
    =================== */

file_t* vfs_open(const char* path, int flags, umode_t mode)
{
    if (!path || path[0] == '\0')
        return NULL;

    vnode_t* vnode;
    int      res = iname_lookup(path, NULL, &vnode);
    if (res)
        return NULL;

    file_t* file = file_create(vnode, mode);
    vnode_dec_ref(vnode);
    if (!file)
        return NULL;

    if (vnode->v_ops && vnode->v_ops->open) {
        if (vnode->v_ops->open(vnode, file)) {
            file_dec_ref(file);
            return NULL;
        }
    }

    return file;
}

void vfs_close(file_t* file)
{
    if (!file)
        return;

    file_dec_ref(file);
}

ssize_t vfs_read(file_t* file, void __user* buf, size_t count, size_t offset)
{
    if (!file || !buf)
        return -EINVAL;
    if (!file->f_ops || !file->f_ops->read)
        return -EBADF;
    if (!(file->f_mode & FMODE_READ))
        return -EACCES;

    loff_t pos = offset ? offset : file->f_pos;

    loff_t saved = file->f_pos;
    file->f_pos = pos;

    int bytes = file->f_ops->read(file, buf, count);

    if (offset == 0 && bytes > 0)
      ;
    else
        file->f_pos = saved; // Restore original position if using offset or if read failed

    return bytes;
}

ssize_t vfs_write(file_t* file, const void __user* buf, size_t count, size_t offset)
{
    if (!file || !buf)
        return -EINVAL;
    if (!file->f_ops || !file->f_ops->write)
        return -EBADF;
    if (!(file->f_mode & FMODE_WRITE))
        return -EACCES;

    loff_t saved = file->f_pos;
    if (offset)
        file->f_pos = offset;

    int bytes = file->f_ops->write(file, buf, count);

    if (offset == 0 && bytes > 0)
        ;
    else
        file->f_pos = saved; // Restore original position if using offset or if write failed

    return bytes;
}

int vfs_llseek(file_t* file, loff_t offset, int whence)
{
    if (!file)
        return -1;
    if (!file->f_ops || !file->f_ops->seek)
        return -1;

    return file->f_ops->seek(file, offset, whence); 
}

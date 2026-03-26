#include "file.h"
#include "vnode.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

file_ops_t regular_file_ops = {.llseek         = regular_file_llseek,
                               .read           = regular_file_read,
                               .write          = regular_file_write,
                               .open           = regular_file_open,
                               .close          = regular_file_close,
                               .ioctl          = DISALLOWED_OP,
                               .mmap           = DISALLOWED_OP,
                               .fsync          = DISALLOWED_OP,
                               .getattr        = DISALLOWED_OP,
                               .setattr        = DISALLOWED_OP,
                               .iterate_shared = DISALLOWED_OP};

void file_inc_ref(file_t* file)
{
    __sync_fetch_and_add(&file->ref_count, 1);
}

void file_dec_ref(file_t* file)
{
    if (__sync_fetch_and_sub(&file->ref_count, 1) == 1) {
        file_destroy(file);
    }
}

file_t* file_create(vnode_t* vnode, fmode_t mode, file_ops_t* f_ops)
{
    if (!vnode || !f_ops)
        return NULL;

    file_t* file = kmalloc(sizeof(file_t));
    if (!file)
        return NULL;

    file->f_vnode   = vnode;
    file->f_mode    = mode;
    file->f_ops     = f_ops;
    file->f_pos     = 0;
    file->ref_count = 1;

    return file;
}

void file_destroy(file_t* file)
{
    if (!file)
        return;

    // Call release operation if provided
    if (file->f_ops && file->f_ops->close)
        file->f_ops->close(file);

    kfree(file);
}

int regular_file_open(file_t* file)
{
    return 0;
}

int regular_file_close(file_t* file)
{
    return 0;
}

int regular_file_llseek(file_t* file, loff_t offset, int whence)
{
    switch (whence) {
    case SEEK_SET:
        file->f_pos = offset;
        break;
    case SEEK_CUR:
        file->f_pos += offset;
        break;
    case SEEK_END:
        // We would need the file size to implement this properly
        return -EINVAL;
    default:
        return -EINVAL;
    }

    return 0; // Success
}

int regular_file_read(file_t* file, char* buf, size_t count, size_t offset)
{
    size_t bytes_read = file->f_vnode->v_ops->read(file->f_vnode, buf, count, offset);
    if (is_errno(bytes_read))
        return bytes_read;

    file->f_pos += bytes_read;
    return bytes_read;
}

int regular_file_write(file_t* file, const char* buf, size_t count, size_t offset)
{
    ssize_t bytes_written = file->f_vnode->v_ops->write(file->f_vnode, buf, count, offset);
    if (is_errno(bytes_written))
        return bytes_written;

    file->f_pos += bytes_written;
    return bytes_written;
}

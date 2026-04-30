#include "file.h"
#include "vnode.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

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

file_t* file_create(vnode_t* vnode, fmode_t mode)
{
    if (!vnode)
        return NULL;

    file_t* file = kmalloc(sizeof(file_t));
    if (!file)
        return NULL;

    file->f_vnode   = vnode;
    file->f_mode    = mode;
    file->f_pos     = 0;
    file->ref_count = 1;
    file->f_ops     = NULL;
    file->private   = NULL;

    vnode_inc_ref(vnode); // Increment ref count for the vnode since the file will hold a reference

    return file;
}

void file_destroy(file_t* file)
{
    if (!file)
        return;

    if (file->f_ops && file->f_ops->close)
        file->f_ops->close(file);
    else if (file->f_vnode && file->f_vnode->v_ops && file->f_vnode->v_ops->close)
        file->f_vnode->v_ops->close(file->f_vnode);

    if (file->f_vnode)
        vnode_dec_ref(file->f_vnode);

    kfree(file);
}

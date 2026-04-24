#ifndef FS_FILE_H
#define FS_FILE_H

#include "types.h"

#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct file {
    vnode_t* f_vnode;
    fmode_t  f_mode;
    loff_t   f_pos;
    int      ref_count;
    void* private; // For filesystem-specific data
} file_t;

void file_inc_ref(file_t* file);
void file_dec_ref(file_t* file);

file_t* file_create(vnode_t* vnode, fmode_t mode);
void    file_destroy(file_t* file);

#endif // FS_FILE_H

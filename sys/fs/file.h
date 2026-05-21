#ifndef FS_FILE_H
#define FS_FILE_H

#include "types.h"

#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct file_ops {
    ssize_t (*read)(file_t* file, void* buf, size_t count);
    ssize_t (*write)(file_t* file, const void* buf, size_t count);
    int (*ioctl)(file_t* file, int cmd, void* arg);
    int (*close)(file_t* file);
    int (*seek)(file_t* file, loff_t offset, int whence);
} file_ops_t;

typedef struct file {
    file_ops_t* f_ops;
    vnode_t*    f_vnode;
    fmode_t     f_mode;
    loff_t      f_pos;
    int         ref_count;
    void* private; // For filesystem-specific data
} file_t;

#define CREATE_FILE_OPS(name)                                                                      \
    int               name##_file_read(file_t* file, void* buf, size_t count);                     \
    int               name##_file_write(file_t* file, const void* buf, size_t count);              \
    int               name##_file_ioctl(file_t* file, int cmd, void* arg);                         \
    int               name##_file_close(file_t* file);                                             \
    int               name##_file_seek(file_t* file, loff_t offset, int whence);                   \
    extern file_ops_t name##_file_ops;

void file_inc_ref(file_t* file);
void file_dec_ref(file_t* file);

file_t* file_create(vnode_t* vnode, fmode_t mode);
void    file_destroy(file_t* file);

#endif // FS_FILE_H

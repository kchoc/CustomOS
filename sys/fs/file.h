#ifndef FS_FILE_H
#define FS_FILE_H

#include "types.h"

#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct file_ops {
    int (*llseek)           (file_t* file, size_t offset, int whence);
    int (*read)             (file_t* file,       char* buf, size_t count, size_t offset);
    int (*write)            (file_t* file, const char* buf, size_t count, size_t offset);
    int (*open)             (file_t* file);
    int (*close)            (file_t* file);
    int (*ioctl)            (file_t* file, unsigned long request, void* arg);
    int (*mmap)             (file_t* file, void* addr, size_t length, int prot, int flags, size_t offset);
    int (*fsync)            (file_t* file);
    int (*getattr)          (file_t* file,        void* statbuf);
    int (*setattr)          (file_t* file,  const void* statbuf);
    int (*iterate_shared)   (file_t* file, dir_context_t* ctx);
    // other operations ...
} file_ops_t;

typedef struct file {
  vnode_t 		*f_vnode;
	fmode_t 		f_mode;
	file_ops_t 	*f_ops;
	loff_t 			f_pos;
	int 				ref_count;
  void*       private; // For filesystem-specific data
} file_t;

void file_inc_ref(file_t* file);
void file_dec_ref(file_t* file);

file_t* file_create(vnode_t* vnode, fmode_t mode, file_ops_t* f_ops);
void file_destroy(file_t* file);

#define CREATE_FILE_OPS(name) \
  int name##_file_llseek( file_t* file, size_t offset, int whence); \
  int name##_file_read(   file_t* file,       char* buf, size_t count, size_t offset); \
  int name##_file_write(  file_t* file, const char* buf, size_t count, size_t offset); \
  int name##_file_open(   file_t* file); \
  int name##_file_close(  file_t* file); \
  int name##_file_ioctl(  file_t* file, unsigned long request, void* arg); \
  int name##_file_mmap(   file_t* file, void* addr, size_t length, int prot, int flags, loff_t offset); \
  int name##_file_fsync(  file_t* file); \
  int name##_file_getattr(file_t* file,       void* statbuf); \
  int name##_file_setattr(file_t* file, const void* statbuf); \
  int name##_file_iterate_shared(file_t* file, dir_context_t* ctx); \
  extern file_ops_t name##_file_ops;

CREATE_FILE_OPS(regular);

#endif // FS_FILE_H

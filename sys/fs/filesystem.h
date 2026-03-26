#ifndef FS_FILESYSTEM_H
#define FS_FILESYSTEM_H

#include "types.h"

#define MAX_FILESYSTEMS 16
#define MAX_FILESYSTEM_NAME_LEN 32

typedef struct file_system_type {
  char name[MAX_FILESYSTEM_NAME_LEN];
  mount_ops_t* fs_ops;
} file_system_type_t;

int register_filesystem(file_system_type_t* fs_type);
int unregister_filesystem(const char* name);

int get_filesystem_type(const char* name, file_system_type_t** result);

#endif // FS_FILESYSTEM_H

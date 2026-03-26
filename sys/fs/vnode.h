#ifndef VFS_VNODE_H
#define VFS_VNODE_H

#include "types.h"

#include <inttypes.h>
#include <stddef.h>

typedef enum vnode_type {
  VNODE_TYPE_FILE,
  VNODE_TYPE_DIRECTORY,
  VNODE_TYPE_SYMLINK,
  VNODE_TYPE_BLOCK_DEVICE,
  VNODE_TYPE_CHAR_DEVICE,
  VNODE_TYPE_SOCKET,
  VNODE_TYPE_FIFO,
} vnode_type_t;

typedef enum vnode_flags {
  VNODE_FLAG_NONE = 0x0,
  VNODE_FLAG_MOUNT_POINT = 0x1, // Indicates this vnode is a mount point
} vnode_flags_t;

typedef enum access_mode {
  ACCESS_READ = 0x1,
  ACCESS_WRITE = 0x2,
  ACCESS_EXECUTE = 0x4,
} access_mode_t;

typedef int(*lookup_fn)(struct vnode* dir, const char* name, struct vnode** result);
typedef int(*create_fn)(struct vnode* dir, const char* name, vmode_t mode, struct vnode** result);
typedef int(*link_fn)(struct vnode* dir, struct vnode* target, const char* name);
typedef int(*unlink_fn)(struct vnode* dir, const char* name);
typedef int(*rename_fn)(struct vnode* old_dir, const char* old_name, struct vnode* new_dir, const char* new_name);
typedef int(*mkdir_fn)(struct vnode* dir, const char* name, vmode_t mode);
typedef int(*rmdir_fn)(struct vnode* dir, const char* name);
typedef int(*readdir_fn)(struct vnode* dir, void* buf, size_t size, size_t offset);
typedef int(*open_fn)(struct vnode* node, fmode_t mode);
typedef int(*close_fn)(struct vnode* node);
typedef int(*read_fn)(struct vnode* node, void* buf, size_t size, size_t offset);
typedef int(*write_fn)(struct vnode* node, const void* buf, size_t size, size_t offset);
typedef int(*getattr_fn)(struct vnode* node, stat_t* st);
typedef int(*setattr_fn)(struct vnode* node, const stat_t* st);
typedef int(*truncate_fn)(struct vnode* node, size_t size);
typedef int(*access_fn)(struct vnode* node, access_mode_t mode);
typedef int(*symlink_fn)(struct vnode* dir, const char* name, const char* target);
typedef int(*readlink_fn)(struct vnode* node, char* buf, size_t size);
typedef int(*mknod_fn)(struct vnode* dir, const char* name, vmode_t mode, uint64_t dev);
typedef int(*fsync_fn)(struct vnode* node);
typedef int(*inactive_fn)(struct vnode* node);
typedef int(*reclaim_fn)(struct vnode* node);

typedef struct vnode_ops {
  lookup_fn     lookup;
  create_fn     create;
  link_fn       link;
  unlink_fn     unlink;
  rename_fn     rename;
  mkdir_fn      mkdir;
  rmdir_fn      rmdir;
  readdir_fn    readdir;
  open_fn       open;
  close_fn      close;
  read_fn       read;
  write_fn      write;
  getattr_fn    getattr;
  setattr_fn    setattr;
  truncate_fn   truncate;
  access_fn     access;
  symlink_fn    symlink;
  readlink_fn   readlink;
  mknod_fn      mknod;
  fsync_fn      fsync; // Optional, can be NULL if not supported
  inactive_fn    inactive; // Called when vnode becomes inactive (ref count drops to zero)
  reclaim_fn     reclaim; // Called to reclaim vnode resources before reuse or destruction
} vnode_ops_t;

#define CREATE_VNODE_OPS(fs_name) \
  int fs_name##_vnode_lookup(struct vnode* dir, const char* name, struct vnode** result); \
  int fs_name##_vnode_create(struct vnode* dir, const char* name, vmode_t mode, struct vnode** result); \
  int fs_name##_vnode_link(struct vnode* dir, struct vnode* target, const char* name); \
  int fs_name##_vnode_unlink(struct vnode* dir, const char* name); \
  int fs_name##_vnode_rename(struct vnode* old_dir, const char* old_name, struct vnode* new_dir, const char* new_name); \
  int fs_name##_vnode_mkdir(struct vnode* dir, const char* name, vmode_t mode); \
  int fs_name##_vnode_rmdir(struct vnode* dir, const char* name); \
  int fs_name##_vnode_readdir(struct vnode* dir, void* buf, size_t size, size_t offset); \
  int fs_name##_vnode_open(struct vnode* node, fmode_t mode); \
  int fs_name##_vnode_close(struct vnode* node); \
  int fs_name##_vnode_read(struct vnode* node, void* buf, size_t size, size_t offset); \
  int fs_name##_vnode_write(struct vnode* node, const void* buf, size_t size, size_t offset); \
  int fs_name##_vnode_getattr(struct vnode* node, stat_t* st); \
  int fs_name##_vnode_setattr(struct vnode* node, const stat_t* st); \
  int fs_name##_vnode_truncate(struct vnode* node, size_t size); \
  int fs_name##_vnode_access(struct vnode* node, access_mode_t mode); \
  int fs_name##_vnode_symlink(struct vnode* dir, const char* name, const char* target); \
  int fs_name##_vnode_readlink(struct vnode* node, char* buf, size_t size); \
  int fs_name##_vnode_mknod(struct vnode* dir, const char* name, vmode_t mode, uint64_t dev); \
  int fs_name##_vnode_fsync(struct vnode* node); \
  int fs_name##_vnode_inactive(struct vnode* node); \
  int fs_name##_vnode_reclaim(struct vnode* node); \
  extern vnode_ops_t fs_name##_vnode_ops;

typedef struct vnode {
  vnode_type_t  v_type;
  int           ref_count; // TODO: Active and cached reference counts?
  vnode_ops_t*  v_ops;
  vnode_flags_t v_flags;

  mount_t*      v_mount; // The mount this vnode belongs to
  mount_t*      v_mounthere; // If this vnode is a mount point, this points to the mounted filesystem's mount struct
  
  uint64_t      file_id; // Unique identifier for the file (e.g., inode number)
  void*         v_data; // Filesystem-specific data (e.g., pointer to inode)

  vnode_t*      hash_next; // For hash table chaining in vnode cache
  vnode_t*      mnt_next; // For linked list of vnodes in a mount
} vnode_t;

void vnode_inc_ref(vnode_t* vnode);
void vnode_dec_ref(vnode_t* vnode);

int vnode_get(mount_t* mount, uint64_t file_id, vnode_t** result);
void vnode_inactive(vnode_t* vnode);
void vnode_reclaim(vnode_t* vnode);

#endif // VFS_VNODE_H

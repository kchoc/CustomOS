#ifndef FS_TYPES_H
#define FS_TYPES_H

typedef struct dir_context dir_context_t;

typedef struct file file_t;
typedef struct mount mount_t;
typedef struct vnode vnode_t;
typedef struct file_system_type file_system_type_t;
typedef struct stat stat_t;

typedef struct file_ops file_ops_t;
typedef struct socket_ops socket_ops_t;
typedef struct vnode_ops vnode_ops_t;
typedef struct dentry_ops dentry_ops_t;
typedef struct mount_ops mount_ops_t;

typedef unsigned int loff_t;
typedef unsigned int vmode_t; // Vnode type flags (e.g., VNODE_TYPE_REGULAR, VNODE_TYPE_DIRECTORY,
                              // VNODE_TYPE_BLOCK_DEVICE)
typedef unsigned int umode_t; // File mode (permissions) flags (e.g., S_IRUSR, S_IWUSR, S_IXUSR)
typedef unsigned int fmode_t; // File mode flags (e.g., O_RDONLY, O_WRONLY, O_RDWR)

#define DISALLOWED_OP (void*)op_disallowed
int op_disallowed(void*, ...);

#endif // FS_TYPES_H

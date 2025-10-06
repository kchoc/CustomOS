#ifndef VFS_H
#define VFS_H

#include "kernel/drivers/ide.h"
#include "types/list.h"
#include "kernel/types.h"
#include "kernel/compiler.h"

typedef struct file file_t;
typedef struct inode inode_t;
typedef struct dentry dentry_t;
typedef struct super_block super_block_t;
typedef struct vfsmount vfsmount_t;
typedef struct dir_context dir_context_t;

/* ===========
   DIR CONTEXT
   =========== */
typedef struct dir_context {
    unsigned int pos;
    bool (*actor)(dir_context_t *ctx,
        const char *name, int namelen,
        uint32_t ino, uint32_t file_size, unsigned int type);
} dir_context_t;

/* ================
   SUPER OPERATIONS
   ================ */
typedef struct superblock_operations {
    inode_t*   (*alloc_inode)      (super_block_t* sb);
    void       (*destroy_inode)    (inode_t* inode);
    int        (*write_inode)      (inode_t* inode, int sync);
    void       (*drop_inode)       (inode_t* inode);
    void       (*put_super)        (super_block_t* sb);
    void       (*statfs)           (dentry_t* dentry, void* buf);
} sb_ops_t;

/* ==========
   OPERATIONS
   ========== */
typedef struct inode_operations {
    dentry_t*  (*lookup)           (inode_t* dir, const char* name, unsigned int flags);
    int        (*create)           (inode_t* dir, dentry_t* dentry, umode_t mode, bool excl);
    int        (*link)             (inode_t* dir, dentry_t* old_dentry, dentry_t* new_dentry);
    int        (*unlink)           (inode_t* dir, dentry_t* dentry);
    int        (*mkdir)            (inode_t* dir, dentry_t* dentry, umode_t mode);
    int        (*rmdir)            (inode_t* dir, dentry_t* dentry);
    int        (*rename)           (inode_t* old_dir, dentry_t* old_dentry,
                                    inode_t* new_dir, dentry_t* new_dentry, unsigned int flags);
} inode_ops_t;

typedef struct file_operations {
    loff_t     (*llseek)           (file_t* file, loff_t offset, int whence);
    ssize_t    (*read)             (file_t* file,       char* __user buf, size_t count, loff_t *offset);
    ssize_t    (*write)            (file_t* file, const char* __user buf, size_t count, loff_t *offset);
    int        (*open)             (inode_t* inode, file_t* file);
    int        (*release)          (inode_t* inode, file_t* file);
    int        (*iterate_shared)   (file_t* file, dir_context_t* ctx);
    // other operations ...
} file_ops_t;

typedef struct dentry_operations {
    int        (*d_revalidate)     (dentry_t* dentry, unsigned int flags);
    void       (*d_release)        (dentry_t* dentry);
    char*      (*d_dname)          (dentry_t* dentry, char* buf, int buflen);
} dentry_ops_t;

/* ================
   FILE SYSTEM TYPE
   ================ */
typedef struct file_system_type {
    const char*    name;
    int            fs_flags;
    dentry_t*      (*mount)        (struct file_system_type* fs_type, int flags,
                                    block_device_t* device, void* data);
    void           (*kill_sb)      (super_block_t* sb);
} file_system_type_t;

/* ==============
   VFS STRUCTURES
   ============== */
typedef struct inode {
    umode_t            i_mode;
    unsigned int       i_flags;

    const file_ops_t*  f_ops;
    const inode_ops_t* i_ops;
    super_block_t*     i_sb;

    unsigned long      i_ino;
    loff_t             i_size;
    int                ref_count;
    void*              private;
} inode_t;

typedef struct dentry {
    dentry_t*          d_parent;
    inode_t*           d_inode;
    char*              d_name;
    super_block_t*     d_sb;
    list_node_t        node; // for children list
    list_t             children;
    int                ref_count;
    bool               is_mountpoint;
    super_block_t*     mnt_sb; // if is_mountpoint is true
} dentry_t;

typedef struct super_block {
    unsigned long           block_size;
    unsigned long           s_magic;
    dentry_t*               s_root;
    block_device_t*         device;    
    file_system_type_t*     fs_type; // e.g., "ext4", "fat16"
    const sb_ops_t*         s_op;
    void*                   private;
} super_block_t;

struct vfsmount {
    super_block_t*  sb;
    dentry_t*       root;
    dentry_t*       mountpoint;
};

/* ==========
    VFS MODES
    ========= */

/* File modes */

/* File is open for reading */
#define FMODE_READ      0x1
/* File is open for writing */
#define FMODE_WRITE     0x2
/* File supports seeking */
#define FMODE_LSEEK     0x4
/* File is opened for reading using pread */
#define FMODE_PREAD     0x8
/* File is opened for writing using pwrite */
#define FMODE_PWRITE    0x10
/* File is opened for execution */
#define FMODE_EXEC      0x20

/* Inode modes (file types) */

/* File type mask */
#define UMODE_IFMT   0xF000
/* Regular file */
#define UMODE_IFREG  0x8000
/* Directory */
#define UMODE_IFDIR  0x4000
/* Character device */
#define UMODE_IFCHR  0x2000
/* Block device */
#define UMODE_IFBLK  0x6000
/* FIFO */
#define UMODE_IFIFO  0x1000
/* Symbolic link */
#define UMODE_IFLNK  0xA000
/* Socket */
#define UMODE_IFSOCK 0xC000

/* Block Device Register */
int             vfs_register_block_device(block_device_t* bdev);
block_device_t* vfs_get_block_device(const char* device_name);

/* VFS Structure Allocators */

super_block_t*  alloc_superblock(file_system_type_t* fs_type, const sb_ops_t* s_ops);
dentry_t*       alloc_dentry(const char* name, inode_t* inode, dentry_t* parent);
inode_t*        alloc_inode(unsigned long ino, umode_t mode, loff_t size,
                        const inode_ops_t* i_ops, const file_ops_t* f_ops);
file_t*         alloc_file(dentry_t* dentry, fmode_t mode);

/* Filesystem Types */
file_system_type_t* register_filesystem(file_system_type_t* fs_type);
file_system_type_t* get_fs_type(const char* name);

/* Mounting/Unmounting */
vfsmount_t* alloc_vfsmount(void);
int         free_vfsmount(vfsmount_t* mnt);
vfsmount_t* lookup_mnt_for_dentry(dentry_t* dentry);
int         mount_fs(super_block_t* sb, dentry_t* mountpoint, dentry_t* mnt_root, block_device_t* device);
int         unmount_fs(dentry_t* mountpoint);
void        vfs_unmount(vfsmount_t* mnt);
int         vfs_mount_root(block_device_t* device);
int         vfs_mount_drive(const char* device_name, const char* mount_path, file_system_type_t* fs_type);

/* Dentry cache */
dentry_t* dentry_cache_lookup(dentry_t* parent, const char* name);
void      dentry_cache_release(dentry_t* dentry);

/* Path resolution */
dentry_t* vfs_lookup(dentry_t* start, const char* path);

/* File operations */
file_t* vfs_open(   const char* path, int flags, umode_t mode);
void    vfs_close(  file_t* file);
ssize_t vfs_read(   file_t* file,       void* __user buf, size_t count, loff_t* offset);
ssize_t vfs_write(  file_t* file, const void* __user buf, size_t count, loff_t* offset);
int     vfs_llseek( file_t* file, loff_t offset, int whence);

/* Directory operations */
int vfs_mkdir(  inode_t* dir, dentry_t* dentry, umode_t mode);
int vfs_rmdir(  inode_t* dir, dentry_t* dentry);
int vfs_create( inode_t* dir, dentry_t* dentry, umode_t mode, bool excl);
int vfs_unlink( inode_t* dir, dentry_t* dentry);
int vfs_rename( inode_t* old_dir, dentry_t* old_dentry,
                inode_t* new_dir, dentry_t* new_dentry, unsigned int flags);

/* Misc */
void vfs_print_mounts(void);
void vfs_print_tree(dentry_t* dir, int depth);
void vfs_ls(const char* path);

#endif // VFS_H

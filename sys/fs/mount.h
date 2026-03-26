#ifndef MOUNT_H
#define MOUNT_H

#include "types.h"

#include <kern/spinlock.h>

#define MAX_MOUNT_NAME_LEN 256

typedef enum mount_flags {
  MOUNT_NONE     = 0x0,
  MOUNT_READONLY = 0x1,
  MOUNT_NOEXEC   = 0x2,
  MOUNT_NOSUID   = 0x4,
  MOUNT_NODEV    = 0x8,
} mount_flags_t;

typedef struct mount_ops {
  int (*mount)(mount_t* mnt, const char* options);
  int (*unmount)(mount_t* mnt);
  int (*get_root)(mount_t* mnt, vnode_t** vnode);
  int (*sync)(mount_t* mnt);
} mount_ops_t;

typedef struct mount {
  vnode_t*        mnt_point; // e.g., "/mnt/usb
  vnode_t*        mnt_dev_vnode;   // The vnode representing the device being mounted (e.g., block device)
  void*           private; // Filesystem-specific data
  mount_ops_t*   mnt_ops;

  mount_flags_t   mnt_flags;
  int             ref_count;

  // Mount's vnodes
  int             vnode_count;
  vnode_t*        vnode_list;
  spinlock_t      vnode_list_lock;

  int             lazy_vnode_count; // Number of vnodes that are lazily created but not yet fully initialized
  vnode_t*        lazy_vnode_list; // List of vnodes that are lazily created but not yet fully initialized
  spinlock_t      lazy_vnode_list_lock;

  struct mount*   mnt_parent;
} mount_t;

#define MAX_MOUNTS 128

extern mount_t* root_mnt;
extern mount_t* mount_table[];
extern int mount_count;

void mount_inc_ref(mount_t* mnt);
void mount_dec_ref(mount_t* mnt);

int mount_create(vnode_t* mnt_point, vnode_t* mnt_dev_vnode, mount_flags_t flags, mount_t** result);
int mount_destroy(mount_t* mnt);

mount_t* lookup_mnt_by_vnode(vnode_t *vnode);

#endif // MOUNT_H


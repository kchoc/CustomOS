#ifndef FS_VNODE_CACHE_H
#define FS_VNODE_CACHE_H

#include "types.h"

#include <kern/spinlock.h>

#include <inttypes.h>

#define MAX_VNODE_CACHE_SIZE 1024

typedef struct vnode_cache_bucket {
  vnode_t* head; // Head of the linked list for this bucket
  spinlock_t lock; // Lock to protect this bucket
} vnode_cache_bucket_t;

int vnode_cache_init(void);
int vnode_cache_insert(vnode_t* vnode);
int vnode_cache_remove(vnode_t* vnode);
int vnode_cache_lookup(mount_t* mnt, uint64_t file_id, vnode_t** result);

#endif // FS_VNODE_CACHE_H

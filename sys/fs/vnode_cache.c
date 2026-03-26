#include "vnode_cache.h"
#include "vnode.h"

#include <kern/spinlock.h>
#include <kern/terminal.h>
#include <kern/errno.h>

#include <string.h>

vnode_cache_bucket_t node_cache[MAX_VNODE_CACHE_SIZE];

int vnode_cache_init(void) {
  memset(node_cache, 0, sizeof(node_cache)); 
  return 0;
}

int vnode_cache_insert(vnode_t* vnode) {
  if (!vnode) return -EINVAL;

  uint64_t key = (uint32_t)vnode->v_mount ^ vnode->file_id; // Simple hash key based on mount and file ID
  int index = key % MAX_VNODE_CACHE_SIZE;

  WITH_SPINLOCK(node_cache[index].lock);

  vnode->hash_next = node_cache[index].head;
  node_cache[index].head = vnode;
  
  END_WITH_SPINLOCK

  return 0; // Success
}

int vnode_cache_remove(vnode_t* vnode) {
  if (!vnode) return -EINVAL;

  uint64_t key = (uint32_t)vnode->v_mount ^ vnode->file_id;
  int index = key % MAX_VNODE_CACHE_SIZE;

  WITH_SPINLOCK(node_cache[index].lock);
  
  vnode_t* current = node_cache[index].head;
  vnode_t* prev = NULL;
  while (current) {
    if (current == vnode) {
      if (prev) {
        prev->hash_next = current->hash_next;
      } else {
        node_cache[index].head = current->hash_next;
      }
      return 0; // Success
    }
    prev = current;
    current = current->hash_next;
  }
  
  END_WITH_SPINLOCK

  return -ENOENT; // Not found
}

int vnode_cache_lookup(mount_t* mnt, uint64_t file_id, vnode_t** result) {
  if (!mnt || !result) return -EINVAL;

  uint64_t key = (uint32_t)mnt ^ file_id;
  int index = key % MAX_VNODE_CACHE_SIZE;

  WITH_SPINLOCK(node_cache[index].lock);

  vnode_t* current = node_cache[index].head;
  while (current) {
    if (current->v_mount == mnt && current->file_id == file_id) {
      *result = current;
      return 0; // Success
    }
    current = current->hash_next;
  }
  
  END_WITH_SPINLOCK

  return -ENOENT; // Not found
}


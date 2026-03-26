#ifndef FS_VFAT_VNODE_H
#define FS_VFAT_VNODE_H

#include <fs/vnode.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct vfat_node_data {
  uint32_t  start_cluster;
  uint32_t  file_size;
  uint8_t   attributes;
  bool      is_root;
  uint32_t  cached_cluster;
  uint32_t  cached_index;
} vfat_node_data_t;

CREATE_VNODE_OPS(vfat);

#endif // FS_VFAT_VNODE_H

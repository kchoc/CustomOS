#ifndef FS_VFAT_VNODE_H
#define FS_VFAT_VNODE_H

#include <fs/vnode.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum vfat_attributes {
    VFAT_ATTR_READ_ONLY = 0x01,
    VFAT_ATTR_HIDDEN    = 0x02,
    VFAT_ATTR_SYSTEM    = 0x04,
    VFAT_ATTR_VOLUME_ID = 0x08,
    VFAT_ATTR_DIR       = 0x10,
    VFAT_ATTR_ARCHIVE   = 0x20,
    VFAT_ATTR_LFN       = 0x0F,
} vfat_attributes_t;

typedef struct vfat_node_data {
    uint32_t start_cluster;
    uint32_t file_size;
    uint8_t  attributes;
    bool     is_root;
    uint32_t cached_cluster;
    uint32_t cached_index;
} vfat_node_data_t;

CREATE_VNODE_OPS(vfat);

#endif // FS_VFAT_VNODE_H

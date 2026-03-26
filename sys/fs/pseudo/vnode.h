#ifndef FS_PSEUDO_VNODE_H
#define FS_PSEUDO_VNODE_H

#include <fs/vnode.h>

#include <list.h>

typedef struct pseudo_vnode_data {
    list_t children; // For directory-like behavior (can be empty for non-directories)
} pseudo_vnode_data_t;

typedef struct pseudo_vnode_list_entry {
    list_node_t node; // For linking in a list of pseudo vnodes
    char*       name; // Name of the entry (for lookup)
    vnode_t*    vnode;
} pseudo_vnode_list_entry_t;

extern vnode_ops_t pseudo_vnode_ops;

pseudo_vnode_data_t* pseudo_vnode_data_create();
int                  pseudo_vnode_data_destroy(pseudo_vnode_data_t* data);

CREATE_VNODE_OPS(pseudo);

#endif // FS_PSEUDO_VNODE_H

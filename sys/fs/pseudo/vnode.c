// #include "vnode.h"
//
// #include <vm/kmalloc.h>
//
// #include <kern/errno.h>
//
// #include <string.h>
//
// #define NODE_TO_LIST_ENTRY(n) ((pseudo_vnode_list_entry_t*)(n -
// offsetof(pseudo_vnode_list_entry_t, node))) #define NODE_TO_PSEUDO(n)
// (NODE_TO_LIST_ENTRY(n)->vnode)
//
// pseudo_vnode_data_t* pseudo_vnode_data_create() {
//     pseudo_vnode_data_t* data = kmalloc(sizeof(pseudo_vnode_data_t));
//     if (!data) return NULL;
//
//     list_init(&data->children, 0);
//
//     return data;
// }
//
// int pseudo_vnode_data_destroy(pseudo_vnode_data_t* data) {
//     if (!data) return -EINVAL;
//
//     kfree(data);
//     return 0;
// }
//
// pseudo_vnode_list_entry_t* pseudo_vnode_list_entry_create(vnode_t* vnode, const char* name) {
//     if (!vnode) return NULL;
//
//     pseudo_vnode_list_entry_t* entry = kmalloc(sizeof(pseudo_vnode_list_entry_t));
//     if (!entry) return NULL;
//
//     entry->name = strdup(name);
//     entry->vnode = vnode;
//     return entry;
// }
//
// int pseudo_vnode_list_entry_destroy(pseudo_vnode_list_entry_t* entry) {
//     if (!entry) return -EINVAL;
//
//     kfree(entry);
//     return 0;
// }
//
// int pseudo_vnode_lookup(vnode_t* dir, const char* name, vnode_t** result) {
//     if (!dir || !name || !result) return -EINVAL;
//
//     list_node_t* node;
//     list_for_each(node, &((pseudo_vnode_data_t*)dir->v_data)->children) {
//         pseudo_vnode_list_entry_t* entry = NODE_TO_LIST_ENTRY(node);
//         if (strcmp(entry->name, name) == 0) {
//             *result = entry->vnode;
//             vnode_inc_ref(*result); // Increment ref count for the result
//             return 0;
//         }
//     }
//     return -ENOENT; // Not found
// }
//
// int psuedo_vnode_open(vnode_t* node, int flags) {
//     return -EACCES; // Not openable
// }
//
// int pseudo_vnode_release(vnode_t* node) {
//     pseudo_vnode_data_t* data = (pseudo_vnode_data_t*)node->v_data;
//     if (!data) return -1;
//     // Release all children
//     list_node_t* node_iter;
//     while ((node_iter = list_pop_head(&data->children)) != NULL) {
//         vnode_t* child_vnode = NODE_TO_PSEUDO(node_iter);
//         vnode_dec_ref(child_vnode); // Release the vnode reference
//         pseudo_vnode_list_entry_destroy(NODE_TO_LIST_ENTRY(node_iter)); // Free the list entry
//     }
//     return pseudo_vnode_data_destroy(data);
// }
//
// int pseudo_vnode_read(vnode_t* node, void* buf, size_t size, size_t offset) {
//     return -EACCES; // Not readable
// }
//
// int pseudo_vnode_write(vnode_t* node, const void* buf, size_t size, size_t offset) {
//     return -EACCES; // Not writable
// }
//
// int pseudo_vnode_create(vnode_t* dir, const char* name, enum vnode_type type, vnode_t** result) {
//     if (!dir || !name || !result) return -1;
//     if (type != VNODE_TYPE_FILE && type != VNODE_TYPE_DIRECTORY) return -EINVAL;
//
//     pseudo_vnode_data_t* data = pseudo_vnode_data_create();
//     if (!data) return -ENOMEM;
//
//     vnode_t* new_vnode = vnode_create(dir->v_mount, type, &pseudo_vnode_ops, data);
//     if (!new_vnode) {
//         pseudo_vnode_data_destroy(data);
//         return -ENOMEM;
//     }
//
//     vnode_inc_ref(new_vnode); // Hold a reference for the data
//
//     pseudo_vnode_list_entry_t* entry = pseudo_vnode_list_entry_create(new_vnode, name);
//     if (!entry) {
//         vnode_dec_ref(new_vnode); // Release the vnode reference which will free the data
//         return -ENOMEM;
//     }
//
//     list_push_tail(&((pseudo_vnode_data_t*)dir->v_data)->children, &entry->node);
//
//     *result = new_vnode;
//     return 0;
// }
//
// int pseudo_vnode_remove(vnode_t* node, const char* name) {
//     if (!node || !name) return -1;
//     list_node_t* node_iter;
//     list_for_each(node_iter, &((pseudo_vnode_data_t*)node->v_data)->children) {
//         pseudo_vnode_list_entry_t* entry = NODE_TO_LIST_ENTRY(node_iter);
//         if (strcmp(entry->name, name) == 0) {
//             // Found the entry to Remove
//             list_remove(node_iter);
//             vnode_dec_ref(entry->vnode); // Release the vnode reference
//             pseudo_vnode_list_entry_destroy(entry); // Free the list entry
//             return 0;
//         }
//     }
//     return -ENOENT; // Not found
// }
//
// int pseudo_vnode_link(vnode_t* dir, vnode_t* target, const char* name) {
//     return -EACCES; // Can't link
// }
//
// int pseudo_vnode_mkdir(vnode_t* dir, const char* name) {
//     vnode_t* new_dir;
//     return pseudo_vnode_create(dir, name, VNODE_TYPE_DIRECTORY, &new_dir);
// }
//
// int pseudo_vnode_rmdir(vnode_t* dir, const char* name) {
//     return pseudo_vnode_remove(dir, name);
// }
//
// int pseudo_vnode_readdir(vnode_t* dir, dir_context_t* ctx) {
//     return -EACCES; // Not readable as a directory
// }
//
// int pseudo_vnode_getattr(vnode_t* node, stat_t* st) {
//     return -EACCES; // Can't get attributes
// }
//
// int pseudo_vnode_setattr(vnode_t* node, const stat_t* st) {
//     return -EACCES; // Can't set attributes
// }
//
// int pseudo_vnode_rename(vnode_t* old_dir, const char* old_name, vnode_t* new_dir, const char*
// new_name) {
//     if (!old_dir || !old_name || !new_dir || !new_name) return -1;
//
//     list_node_t* node_iter;
//     list_for_each(node_iter, &((pseudo_vnode_data_t*)old_dir->v_data)->children) {
//         pseudo_vnode_list_entry_t* entry = NODE_TO_LIST_ENTRY(node_iter);
//         if (strcmp(entry->name, old_name) == 0) {
//             // Found the entry to rename
//             // Remove from old directory
//             list_remove(node_iter);
//
//             // Update the name in the vnode data
//             kfree(entry->name);
//             entry->name = strdup(new_name);
//
//             // Add to new directory
//             list_push_tail(&((pseudo_vnode_data_t*)new_dir->v_data)->children, &entry->node);
//
//             return 0;
//         }
//     }
//     return -ENOENT; // Not found
// }
//
// int pseudo_vnode_symlink(vnode_t* dir, const char* name, const char* target) {
//     return -EACCES; // Can't create symlinks
// }
//
// int pseudo_vnode_readlink(vnode_t* node, char* buf) {
//     return -EACCES; // Not a symlink
// }
//
// int pseudo_vnode_access(vnode_t* node, int mode) {
//     return -EACCES; // No access permissions
// }
//
// vnode_ops_t pseudo_vnode_ops = {
//     .lookup = pseudo_vnode_lookup,
//     .open = psuedo_vnode_open,
//     .release = pseudo_vnode_release,
//     .read = pseudo_vnode_read,
//     .write = pseudo_vnode_write,
//     .create = pseudo_vnode_create,
//     .remove = pseudo_vnode_remove,
//     .link = pseudo_vnode_link,
//     .mkdir = pseudo_vnode_mkdir,
//     .rmdir = pseudo_vnode_rmdir,
//     .readdir = pseudo_vnode_readdir,
//     .getattr = pseudo_vnode_getattr,
//     .setattr = pseudo_vnode_setattr,
//     .rename = pseudo_vnode_rename,
//     .symlink = pseudo_vnode_symlink,
//     .readlink = pseudo_vnode_readlink,
//     .access = pseudo_vnode_access
// };

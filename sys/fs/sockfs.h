#ifndef SOCKFS_H
#define SOCKFS_H

#include "vfs.h"
#include <kern/socket.h>

#include <inttypes.h>

/* Filesystem type declaration */
extern file_system_type_t sockfs_fs_type;

/* Initialization */
int sockfs_init(void);

/* Mount/unmount operations */
dentry_t* sockfs_mount(file_system_type_t* fs_type, int flags,
                       block_device_t* dev_name, void* data);
void sockfs_kill_sb(super_block_t* sb);

/* Socket management API */
dentry_t* sockfs_create_socket(const char* name, sock_type_t type);
dentry_t* sockfs_lookup_socket(const char* name);
int sockfs_unlink_socket(const char* name);
const socket_ops_t* sockfs_get_socket_ops(void);
void sockfs_list_sockets(void);

#endif //SOCKFS_H
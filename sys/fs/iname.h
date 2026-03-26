#ifndef FS_INAME_H
#define FS_INAME_H

#include "types.h"

#define MAX_INAME_LEN 256

int iname_lookup(const char* name, vnode_t* dir, vnode_t** result);

#endif // FS_INAME_H

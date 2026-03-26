#include "filesystem.h"

#include <kern/spinlock.h>
#include <kern/errno.h>

#include <string.h>

file_system_type_t* fs_types[MAX_FILESYSTEMS];
int fs_type_count = 0;
spinlock_t fs_types_lock = 0;

int register_filesystem(file_system_type_t* fs_type) {
    if (!fs_type || !fs_type->name[0]) return -EINVAL;

    WITH_SPINLOCK(fs_types_lock)

    if (fs_type_count >= MAX_FILESYSTEMS) return -EMFILE; // Too many filesystems

    for (int i = 0; i < fs_type_count; i++) {
        if (strcmp(fs_types[i]->name, fs_type->name) == 0) {
            return -EEXIST; // Filesystem with this name already exists
        }
    }

    fs_types[fs_type_count++] = fs_type;

    END_WITH_SPINLOCK
    return 0; // Success
}

int unregister_filesystem(const char* name) {
    if (!name || !name[0]) return -EINVAL;

    WITH_SPINLOCK(fs_types_lock)
    for (int i = 0; i < fs_type_count; i++) {
        if (strcmp(fs_types[i]->name, name) == 0) {
            // Shift remaining types down to fill the gap
            for (int j = i; j < fs_type_count - 1; j++) {
                fs_types[j] = fs_types[j + 1];
            }
            fs_types[--fs_type_count] = NULL; // Clear the last entry
            return 0; // Success
        }
    }
    END_WITH_SPINLOCK

    return -ENOENT; // Filesystem not found
}

int get_filesystem_type(const char* name, file_system_type_t** result) {
    if (!name || !result) return -EINVAL;

    WITH_SPINLOCK(fs_types_lock)
    for (int i = 0; i < fs_type_count; i++) {
        if (strcmp(fs_types[i]->name, name) == 0) {
            *result = fs_types[i];
            return 0; // Success
        }
    }
    END_WITH_SPINLOCK

    return -ENOENT; // Filesystem not found
}


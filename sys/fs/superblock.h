#ifndef VFS_SUPERBLOCK_H
#define VFS_SUPERBLOCK_H

#include "types.h"

#include <inttypes.h>

typedef struct super_block
{
    // Identification
    uint32_t sb_magic;
    uint32_t sb_version;

    uint64_t block_size;
    uint64_t total_blocks;
    uint64_t free_blocks;

    struct sb_ops* sb_ops;

    void* fs_data;
} super_block_t;

typedef struct sb_ops
{
    int (*syncfs)(super_block_t* sb);
    int (*statfs)(super_block_t* sb, stat_t* st);
    int (*write_super)(super_block_t* sb);
    int (*unmount)(super_block_t* sb);
    int (*remount)(super_block_t* sb, const char* options);
} sb_ops_t;

super_block_t* sb_create(file_system_type_t* fs_type, sb_ops_t* sb_ops);
void sb_destroy(super_block_t* sb);

#endif // VFS_SUPERBLOCK_H

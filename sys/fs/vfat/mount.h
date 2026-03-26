#ifndef FS_VFAT_MOUNT_H
#define FS_VFAT_MOUNT_H

#include "bpb.h"

#include <fs/mount.h>
#include <fs/types.h>

#include <inttypes.h>

typedef enum vfat_flags
{
    VFAT_FLAG_LFN_READ = 0x1,  // Indicates that long filename entries should be read and processed
    VFAT_FLAG_LFN_WRITE = 0x2, // Indicates that long filename entries should be written when
                               // creating files/directories
} vfat_flags_t;

typedef enum fat_type
{
    FAT_TYPE_12 = 0x2,
    FAT_TYPE_16 = 0x3,
    FAT_TYPE_12_16 =
        0x2, // MASK to check if it's FAT12 or FAT16 (if FAT_TYPE_12_16 & 0x2 == 0x2, it's FAT12/16)
    FAT_TYPE_32 = 0x4,
} fat_type_t;

typedef struct vfat_mount_data
{
    bpb_t bpb; // The BIOS Parameter Block read from the disk during mount
    union
    {
        bpb_extension_fat12_16_t fat12_16;
        bpb_extension_fat32_t fat32;
    } bpb_ext;
    uint8_t fat_type; // 12, 16, or 32

    /* --- Layout information calculated from the BPB --- */
    uint32_t fat_start_sector;
    uint32_t fat_size_sectors;
    uint32_t data_start_sector;
    uint32_t root_dir_start_sector; // FAT12/16 Only
    uint32_t root_dir_sectors;      // FAT12/16 Only

    uint32_t root_cluster; // FAT32 Only

    uint32_t cluster_size_bytes;
    uint32_t total_clusters;

    vfat_flags_t flags; // Mount options and state flags
} vfat_mount_data_t;

extern mount_ops_t vfat_mount_ops;

int vfat_mount(mount_t* mnt, const char* options);
int vfat_unmount(mount_t* mnt);
int vfat_get_root(mount_t* mnt, vnode_t** vnode);
int vfat_sync(mount_t* mnt);

#endif // FS_VFAT_MOUNT_H

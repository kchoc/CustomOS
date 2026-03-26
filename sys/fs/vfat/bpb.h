#ifndef FS_VFAT_BPB_H
#define FS_VFAT_BPB_H

#include <stdint.h>

#include <kern/compiler.h>

typedef struct bpb
{
    uint8_t jump_boot[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_short;
    uint8_t media_descriptor;
    uint16_t fat_size_sectors;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_long;
} __packed bpb_t;

typedef struct bpb_extension_fat12_16
{
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
} bpb_extension_fat12_16_t;

typedef struct bpb_extension_fat32
{
    uint32_t fat_size_32; // FAT size in sectors for FAT32
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster; // Starting cluster of the root directory for FAT32
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
} bpb_extension_fat32_t;

#endif /* ifndef F */

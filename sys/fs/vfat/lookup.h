#ifndef FS_VFAT_LOOKUP_H
#define FS_VFAT_LOOKUP_H

#include <fs/types.h>

#include <sys/device.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#define VFAT_LOOKUP_FIND 0x1
#define VFAT_LOOKUP_FREE 0x2

#define IS_CLUSTER_END(mount, cluster) ((mount)->fat_type == FAT_TYPE_32 ? (cluster >= 0x0FFFFFF8) : ((mount)->fat_type == FAT_TYPE_16 ? (cluster >= 0xFFF8) : (cluster >= 0xFF8)))

typedef struct vfat_standard_entry {
  uint8_t   name[11]; // 8.3 filename format
  uint8_t   attributes;
  uint8_t   reserved;
  uint8_t   creation_time_tenths;
  uint16_t  creation_time;
  uint16_t  creation_date;
  uint16_t  last_access_date;
  uint16_t  first_cluster_high; // For FAT32
  uint16_t  last_modification_time;
  uint16_t  last_modification_date;
  uint16_t  first_cluster_low;
  uint32_t  file_size;
} __attribute__((packed)) vfat_standard_entry_t;

typedef struct vfat_lfn_entry {
  uint8_t   order;
  uint16_t  name1[5]; // First 5 characters of the long filename
  uint8_t   attributes; // Always 0x0F for LFN entries
  uint8_t   reserved;
  uint8_t   checksum; // Checksum of the corresponding short filename entry
  uint16_t  name2[6]; // Next 6 characters of the long filename
  uint16_t  first_cluster_low; // Always zero for LFN entries
  uint16_t  name3[2]; // Last 2 characters of the long filename
} __attribute__((packed)) vfat_lfn_entry_t;

typedef struct vfat_lookup_result {
    uint32_t sector;
    size_t offset;

    uint32_t cluster;
    vfat_standard_entry_t entry;
    
    uint32_t lfn_start_sector;
    size_t lfn_start_offset;
    int lfn_count;
    bool has_lfn;
} vfat_lookup_result_t;

typedef struct vfat_mount_data vfat_mount_data_t;

uint64_t cluster_to_lba(vfat_mount_data_t* mount_data, uint16_t cluster);
int follow_cluster_chain(device_t* dev, vfat_mount_data_t* mount_data, uint32_t start_cluster, uint32_t* next_cluster);
void vfat_build_shortname(char* out, const uint8_t* name);

int vfat_lookup(vnode_t* dir, const char* name, int flags, vfat_lookup_result_t* out);

int vfat_free_cluster_chain(device_t* dev, vfat_mount_data_t* mount_data, uint32_t start_cluster);
int vfat_allocate_cluster(device_t* dev, vfat_mount_data_t* mount_data, uint32_t prev_cluster, uint32_t* new_cluster);

#endif // FS_VFAT_LOOKUP_H

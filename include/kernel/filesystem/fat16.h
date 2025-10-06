#ifndef FAT16_H
#define FAT16_H

#include "kernel/drivers/ide.h"
#include "kernel/filesystem/vfs.h"
#include "kernel/types.h"

#define SECTOR_SIZE         512
#define FAT_SECTORS         32
#define FAT_COUNT           2
#define ROOT_ENTRY_COUNT    512
#define ROOT_DIR_SECTORS    ((ROOT_ENTRY_COUNT * 32) / SECTOR_SIZE)
#define RESERVED_SECTORS    1
#define TOTAL_SECTORS       8192  // 4MB disk

#define DATA_CLUSTER_OFFSET (RESERVED_SECTORS + FAT_COUNT * FAT_SECTORS + ROOT_DIR_SECTORS)
#define ROOT_DIR_LBA        (RESERVED_SECTORS + FAT_COUNT * FAT_SECTORS)

#define FAT16_CLUSTER_FREE  0x0000
#define FAT16_CLUSTER_EOF   0xFFFF
#define MAX_CLUSTERS        (TOTAL_SECTORS - DATA_CLUSTER_OFFSET)


#define MAX_PATH_LEN 128
#define ENTRIES_PER_SECTOR (512 / sizeof(fat16_dir_entry_t))

#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

typedef struct __attribute__((packed)) {
    uint8_t  jump_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_short;
    uint8_t  media_descriptor;
    uint16_t fat_size_sectors;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_long;

    // FAT12/16 Extended Boot Record
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    char    volume_label[11];
    char    file_system_type[8];
} bpb_t;

typedef struct __attribute__((packed)) {
    char     name[11];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t ignore_in_fat;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t start_cluster;
    uint32_t file_size;
} fat16_dir_entry_t;

typedef struct fat16_node_info {
    uint16_t start_cluster;
    uint32_t file_size;
    uint32_t dir_lba; // LBA of the directory entry
    uint16_t dir_index; // Index within the sector
} fat16_node_info_t;

typedef struct __attribute__((packed)) {
    uint32_t file_size;
    uint16_t start_cluster;
    uint16_t current_cluster;
    uint32_t parent_dir_lba;
    uint16_t parent_dir_index;
} fat16_file_t;

fat16_node_info_t* alloc_fat16_node_info(uint16_t start_cluster, uint32_t file_size, uint32_t dir_lba, uint16_t dir_index);

dentry_t* fat16_mount(file_system_type_t* fs_type, int flags,
                          block_device_t* dev_name, void* data);
void fat16_kill_sb(super_block_t* sb);

dentry_t*   fat16_lookup(   inode_t* dir, const char* name, unsigned int flags);
int         fat16_create(   inode_t* dir, dentry_t* dentry, umode_t mode, bool excl);
int         fat16_link(     inode_t* dir, dentry_t* old_dentry, dentry_t* new_dentry);
int         fat16_unlink(   inode_t* dir, dentry_t* dentry);
int         fat16_mkdir(    inode_t* dir, dentry_t* dentry, umode_t mode);
int         fat16_rmdir(    inode_t* dir, dentry_t* dentry);
int         fat16_rename(   inode_t* old_dir, dentry_t* old_dentry,
                            inode_t* new_dir, dentry_t* new_dentry, unsigned int flags);


loff_t  fat16_llseek(       file_t* file, loff_t offset, int whence);
ssize_t fat16_read(         file_t* file,       char* __user buf, size_t count, loff_t* offset);
ssize_t fat16_write(        file_t* file, const char* __user buf, size_t count, loff_t* offset);
int     fat16_open(         inode_t* inode, file_t* file);
int     fat16_release(      inode_t* inode, file_t* file);
int     fat16_iterate_shared(file_t* file, dir_context_t* ctx);

// OLD Implementation of FAT16 filesystem
void fat16_flush();

// Initialization
void fat16_init();

// Low-level
uint16_t fat16_alloc_cluster();
void fat16_free_cluster_chain(uint16_t start);
uint16_t fat16_get_next_cluster(uint16_t cluster);
void fat16_set_next_cluster(uint16_t cluster, uint16_t next);
void fat16_set_fat_entry(uint16_t cluster, uint16_t value);

static file_system_type_t fat16_fs_type = {
    .name = "fat16",
    .mount = fat16_mount,
};

#endif //FAT16_H
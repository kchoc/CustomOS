#ifndef FS_H
#define FS_H

#include <stdint.h>

#define SECTOR_SIZE         512
#define FAT_SECTORS         32
#define FAT_COUNT           2
#define ROOT_ENTRY_COUNT    512
#define ROOT_DIR_SECTORS    ((ROOT_ENTRY_COUNT * 32) / SECTOR_SIZE)
#define RESERVED_SECTORS    1
#define TOTAL_SECTORS       8192  // 4MB disk

#define DATA_CLUSTER_OFFSET (RESERVED_SECTORS + FAT_COUNT * FAT_SECTORS + ROOT_DIR_SECTORS)
#define FAT16_CLUSTER_FREE  0x0000
#define FAT16_CLUSTER_EOF   0xFFFF
#define MAX_CLUSTERS        (TOTAL_SECTORS - DATA_CLUSTER_OFFSET)


#define MAX_PATH_LEN 128
#define ENTRIES_PER_SECTOR (512 / sizeof(dir_entry_t))

#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

typedef struct dir dir_t;
typedef struct path path_t;

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
} dir_entry_t;

typedef struct dir {
    dir_entry_t entries[ENTRIES_PER_SECTOR];
    uint16_t cluster;
    dir_t* next; // Pointer to the next sector in the directory chain
} __attribute__((packed)) dir_t;

typedef struct path {
    char name[12];
    path_t* subdir;
} __attribute__((packed)) path_t;

typedef struct fat16_file {
    const char* filename;       // Original filename
    uint32_t size;              // File size in bytes
    uint16_t start_cluster;     // Starting cluster of the file
    uint16_t current_cluster;   // Current cluster being read
    uint16_t cluster_offset;    // Offset within the current cluster
} fat16_file_t;

// Paths
path_t* parse_path(const char* path);
void destroy_path(path_t* path);

// Initialization
void fat16_init();
void fat16_format_if_needed();

// File operations
int fat16_write_file(const char* filename, const uint8_t* data, uint32_t size);
int fat16_read_file(const char* filename, uint8_t** buffer, uint32_t* size);
int fat16_read_file_to_buffer(const char* filename, uint8_t* buffer, uint32_t size);
int fat16_delete_file(const char* path);

fat16_file_t* fat16_open_file(const char* filename);
int fat16_file_read(fat16_file_t* file, uint8_t* buffer, uint32_t size);
int fat16_file_seek(fat16_file_t* file, uint32_t position);
int fat16_file_close(fat16_file_t* file);

// Directory operations
int fat16_free_directory(dir_t* dir);
int fat16_create_dir(const char* name);
void fat16_list_dir();
int fat16_change_directory(const char* path);

// Low-level
uint16_t fat16_alloc_cluster();
void fat16_free_cluster_chain(uint16_t start);
uint16_t fat16_get_next_cluster(uint16_t cluster);
void fat16_set_fat_entry(uint16_t cluster, uint16_t value);

// Utility
int fat16_create_entry(const char* path, uint8_t attr, uint16_t start_cluster, uint32_t size);

#endif //FS_H
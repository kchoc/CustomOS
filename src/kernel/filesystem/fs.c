#include "kernel/filesystem/fs.h"
#include "kernel/drivers/port_io.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/terminal.h"
#include "types/string.h"

#include <stddef.h>
#include <stdint.h>

//TODO:
// Add checkign to exclude filenames starting with : (for root directory)

static uint8_t fat[FAT_SECTORS * SECTOR_SIZE];  // 512B * 32 = 16KB
static dir_t* open_dir = NULL;
static bpb_t* bpb;

static inline uint32_t cluster_to_lba(uint16_t cluster) {
    return DATA_CLUSTER_OFFSET + cluster - 2;
}

static inline void free_open_dir() {
    if (open_dir) {
        if (fat16_free_directory(open_dir))
            printf("Failed to free open directory\n"); // PANIC!!!!!!!
        open_dir = NULL;
    }
}

char* format_filename_83(const char* input) {
    if (!input || strlen(input) > 11) {
        return NULL;
    }

    static char out[12];
    memset(out, ' ', 11);
    const char* dot = strchr(input, '.');

    uint8_t name_len;
    uint8_t ext_len = 0;

    // If it is a file
    if (dot) {
        name_len = (dot - input);
        ext_len = strlen(dot + 1);

        if (name_len > 8) name_len = 8;
        if (ext_len > 3) ext_len = 3;
    } else {
        name_len = strlen(input);
    }

    memcpy(out, input, name_len);
    memcpy(out + 8, dot + 1, ext_len);

    out[11] = '\0';

    strtoupper(out);

    return out;
}

path_t* parse_path(const char* path) {
    if (!path) return NULL; // Check for NULL path
    if (path[0] == '/') path++; // Skip leading slash
    if (path[0] == '\0') return NULL; // Empty path

    const char* slash = strchr(path, '/');
    uint8_t entry_length = (slash) ? (slash - path) : strlen(path);

    if (entry_length > 11)
        return NULL; // Path too long


    path_t* p = kmalloc(sizeof(path_t));
    if (!p) return NULL; // Memory allocation failed
    p->subdir = NULL;
    memset(p->name, ' ', 11);

    memcpy(p->name, path, entry_length);
    p->name[11] = '\0';
    strtoupper(p->name);

    if (!slash)
        memcpy(p->name, format_filename_83(p->name), 12);
    if (slash)
        p->subdir = parse_path(slash + 1);
    
    return p;
}

void destroy_path(path_t* path) {
    if (!path) return;
    if (path->subdir) destroy_path(path->subdir);
    kfree(path);
}

int fat16_validate() {
    if (!bpb || bpb->bytes_per_sector != SECTOR_SIZE || bpb->fat_count != FAT_COUNT ||
        bpb->reserved_sector_count != RESERVED_SECTORS || bpb->total_sectors_short != TOTAL_SECTORS ||
        bpb->fat_size_sectors != FAT_SECTORS || bpb->root_entry_count != ROOT_ENTRY_COUNT) {
        return -1; // Invalid BPB
    }
    return 0; // Valid BPB
}

void fat16_create_boot_sector() {
    // Validate or setup BPB
    memset(bpb, 0, sizeof(bpb_t));

    // Boot sector signature
    bpb->jump_boot[0] = 0xEB;  // JMP short
    bpb->jump_boot[1] = 0x3C;
    bpb->jump_boot[2] = 0x90;

    memcpy(bpb->oem_name, "MSDOS5.0", 8);

    bpb->bytes_per_sector       = SECTOR_SIZE;
    bpb->sectors_per_cluster    = 1;
    bpb->reserved_sector_count  = RESERVED_SECTORS;
    bpb->fat_count              = FAT_COUNT;
    bpb->root_entry_count       = ROOT_ENTRY_COUNT;
    bpb->total_sectors_short    = TOTAL_SECTORS;
    bpb->media_descriptor       = 0xF8;
    bpb->fat_size_sectors       = FAT_SECTORS;
    bpb->sectors_per_track      = 32;
    bpb->head_count             = 64;
    bpb->hidden_sectors         = 0;
    bpb->total_sectors_long     = 0; // not used for FAT12

    bpb->drive_number           = 0x00;
    bpb->reserved               = 0;
    bpb->boot_signature         = 0x29;
    bpb->volume_id              = 0x12345678;
    memcpy(bpb->volume_label, "NO NAME    ", 11);
    memcpy(bpb->file_system_type, "FAT16   ", 8);

    uint8_t* boot_sector = (uint8_t*)bpb;

    boot_sector[510] = 0x55;
    boot_sector[511] = 0xAA;
    ide_write_sector(0, boot_sector);
}

void fat16_flush() {
    for (int i = 0; i < FAT_SECTORS; i++) {
        // Write both copies of the FAT
        ide_write_sector(1 + i, ((uint8_t*)fat) + i * SECTOR_SIZE);
        ide_write_sector(1 + FAT_SECTORS + i, ((uint8_t*)fat) + i * SECTOR_SIZE);
    }
}

void fat16_init() {
    uint8_t* boot_sector = kmalloc(SECTOR_SIZE);
    ide_read_sector(0, boot_sector);
    bpb = (bpb_t*)boot_sector;

    if (fat16_validate())
        fat16_create_boot_sector();

    for (int i = 0; i < FAT_SECTORS; i++)
        ide_read_sector(RESERVED_SECTORS + i, fat + i * SECTOR_SIZE);

    // Check for media descriptor in FAT
    if (fat[0] != 0xF8) {
        fat[0] = 0xF8; // Set media descriptor if not set
        fat16_flush();
    }

    kfree(boot_sector);
}

uint16_t fat16_alloc_cluster() {
    for (uint16_t i = 2; i < 4096; i++) {
        uint16_t val = ((uint16_t*)fat)[i];
        if (val == FAT16_CLUSTER_FREE) {
            ((uint16_t*)fat)[i] = FAT16_CLUSTER_EOF;
            return i;
        }
    }
    return 0;
}

uint16_t fat16_get_next_cluster(uint16_t cluster) {
    return ((uint16_t*)fat)[cluster];
}

void fat16_set_fat_entry(uint16_t cluster, uint16_t value) {
    ((uint16_t*)fat)[cluster] = value;
}

void fat16_free_cluster_chain(uint16_t start) {
    while (start != FAT16_CLUSTER_EOF) {
        uint16_t next = fat16_get_next_cluster(start);
        fat16_set_fat_entry(start, FAT16_CLUSTER_FREE);
        if (next == FAT16_CLUSTER_EOF) break;
        start = next;
    }
}

void fat16_read_cluster_chain(uint16_t start_cluster, uint8_t* buffer, uint32_t max_clusters) {
    uint16_t cluster = start_cluster;
    uint32_t offset = 0;

    while (cluster < FAT16_CLUSTER_EOF && max_clusters--) {
        ide_read_sector(cluster_to_lba(cluster), ((uint8_t*)buffer) + offset);
        offset += SECTOR_SIZE;
        cluster = fat[cluster];
    }
}

int fat16_write_cluster_chain(uint16_t start_cluster, void* buffer, uint32_t clusters) {
    uint16_t current_cluster = start_cluster;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < clusters; i++) {
        if (i > 0) {
            uint16_t next_cluster = fat16_alloc_cluster();
            if (next_cluster == 0) {
                fat16_free_cluster_chain(start_cluster);
                return -1; // No free clusters available
            }
            fat16_set_fat_entry(current_cluster, next_cluster);
            current_cluster = next_cluster;
        }

        ide_write_sector(cluster_to_lba(current_cluster), ((uint8_t*)buffer) + offset);
        offset += SECTOR_SIZE;
    }

    fat16_set_fat_entry(current_cluster, FAT16_CLUSTER_EOF); // Mark the end of the chain
    return 0; // Success
}

int update_directory(dir_t* dir) {
    if (!dir) return -1;

    // Write the directory back to disk
    if (dir->cluster == 0) {
        if (!open_dir)
            return -1; // No directory opened

        // If the directory is the root directory, find it in the open_dir chain
        dir_t* current = open_dir;
        uint16_t i;
        for (i = 0; i < ROOT_ENTRY_COUNT; i++) {
            if (!current)
                return -1; // No directory found in open_dir chain

            if (current == dir)
                break;

            current = current->next;
        }

        if (current != dir)
            return -1; // Directory not found in open_dir chain

        // Write the root directory sector
        ide_write_sector(RESERVED_SECTORS + FAT_SECTORS * FAT_COUNT + i, (uint8_t*)dir);
        return 0;
    }

    ide_write_sector(cluster_to_lba(dir->cluster), (uint8_t*)dir);
    return 0; // Success
}

int fat16_free_directory(dir_t* dir) {
    if (!dir) return -1;

    dir_t* current = dir;
    while (current) {
        dir_t* next = current->next;
        kfree(current);
        current = next;
    }
    return 0; // Success
}

dir_t* fat16_read_directory(uint16_t cluster) {
    uint16_t next = cluster;

    dir_t* first_dir = NULL;
    dir_t* dir = NULL;

    do {
        if (dir) {
            dir->next = kmalloc(sizeof(dir_t));
            dir = dir->next;
        } else {
            dir = kmalloc(sizeof(dir_t));
            first_dir = dir;
        }
        if (!dir) {
            fat16_free_directory(first_dir);
            return NULL; // Memory allocation failed
        }
        dir->cluster = next;
        dir->next = NULL;

        // Read the next sector
        ide_read_sector(cluster_to_lba(next), (uint8_t*)dir->entries);
        
        next = fat16_get_next_cluster(next);
    } while (next < 0xFF8);

    return first_dir;
}

dir_entry_t* fat16_find_in_directory(const char* name, dir_t** dir_sector) {
    dir_t* current_sector = open_dir;

    while (current_sector) {
        for (int i = 0; i < ENTRIES_PER_SECTOR; i++) {
            dir_entry_t* entry = &current_sector->entries[i];
            if (entry->name[0] == 0 || entry->name[0] == (char)0xE5)
                continue;

            if (strncmp(entry->name, name, 11) == 0) {
                if (dir_sector) *dir_sector = current_sector;
                return entry;
            }
        }

        current_sector = current_sector->next;
    }

    return NULL;
}

int fat16_open_root() {
    free_open_dir();
    open_dir = kmalloc(sizeof(dir_t));
    if (!open_dir) return -1; // Memory allocation failed
    open_dir->cluster = 0; // Root directory cluster
    open_dir->next = NULL;

    dir_t* current_sector = open_dir;

    for (uint16_t i = 0; i < ROOT_DIR_SECTORS - 1; i++) {
        // Read the next sector of the root directory
        ide_read_sector(RESERVED_SECTORS + FAT_SECTORS * FAT_COUNT + i, (uint8_t*)current_sector);

        current_sector->next = kmalloc(sizeof(dir_t));
        if (!current_sector->next) {
            fat16_free_directory(open_dir);
            return -1; // Memory allocation failed
        }
        current_sector = current_sector->next;
        current_sector->next = NULL;
        current_sector->cluster = 0; // Not used for root directory
    }

    // Read the last sector of the root directory
    ide_read_sector(RESERVED_SECTORS + FAT_SECTORS * FAT_COUNT + ROOT_DIR_SECTORS - 1, (uint8_t*)current_sector);
    return 0; // Success
}

int fat16_open_directory(path_t* parsed_path) {
    if (!parsed_path) return -1; // Invalid path

    // If the path is empty, open the root directory
    if (parsed_path->name[0] == ':') {
        if (fat16_open_root())
            return -1; // Failed to open root directory

    } else {
        dir_entry_t* entry = fat16_find_in_directory(parsed_path->name, NULL);
        if (!entry || !(entry->attr & ATTR_DIRECTORY))
            return -1; // Not a directory or not found

        uint16_t start_cluster = entry->start_cluster;

        // Free the previously opened directory if it exists
        free_open_dir();

        // Read the directory from the start cluster
        open_dir = fat16_read_directory(start_cluster);

        if (!open_dir)
            return -1; // Failed to read directory
    }

    // If there are subdirectories, recursively open them
    if (parsed_path->subdir)
        return fat16_open_directory(parsed_path->subdir);

    return 0; // Success
}

int fat16_write_file(const char* filename, const uint8_t *data, uint32_t size) {
    fat16_delete_file(filename);

    uint16_t start_cluster = fat16_alloc_cluster();
    if (start_cluster == 0) return -1; // No free clusters available

    if (fat16_create_entry(filename, ATTR_ARCHIVE, start_cluster, size) != 0)
        return -1; // Failed to create file entry

    // Write the data to the allocated cluster chain
    if (fat16_write_cluster_chain(start_cluster, (void*)data, (size + SECTOR_SIZE - 1) / SECTOR_SIZE) != 0) {
        fat16_free_cluster_chain(start_cluster);
        return -1; // Failed to write file data
    }
    fat16_flush();
    return 0;
}

int fat16_read_file(const char* filename, uint8_t** buffer, uint32_t* size) {
    if (!filename || !buffer) return -1; // Invalid parameters

    dir_t* dir_sector = NULL;
    dir_entry_t* entry = fat16_find_in_directory(format_filename_83(filename), &dir_sector);
    if (!entry) return -1; // File not found

    if (!(entry->attr & ATTR_ARCHIVE)) return -1; // Not a file
    if (size) *size = entry->file_size;

    if (entry->file_size == 0) {
        *buffer = NULL; // Empty file
        return 0; // Success
    }

    // Allocate buffer for the file data
    *buffer = kmalloc(entry->file_size);
    if (!*buffer) return -1; // Memory allocation failed

    // Read the file data from the cluster chain
    uint32_t clusters_needed = (entry->file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    fat16_read_cluster_chain(entry->start_cluster, *buffer, clusters_needed);
    return 0; // Success
}

int fat16_read_file_to_buffer(const char* filename, uint8_t* buffer, uint32_t size) {
    if (!filename || !buffer || size == 0) return -1; // Invalid parameters

    dir_t* dir_sector = NULL;
    dir_entry_t* entry = fat16_find_in_directory(format_filename_83(filename), &dir_sector);
    if (!entry) return -1; // File not found

    if (!(entry->attr & ATTR_ARCHIVE)) return -1; // Not a file
    if (size < entry->file_size) return -1; // Buffer too small

    // Read the file data from the cluster chain
    uint32_t clusters_needed = (entry->file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    fat16_read_cluster_chain(entry->start_cluster, buffer, clusters_needed);
    
    return 0; // Success
}

int fat16_delete_file(const char* name) {
    if (!name || strlen(name) == 0) return -1; // Invalid path

    char* name_83 = format_filename_83(name);
    if (!name_83) return -1; // Invalid filename format

    dir_t* dir_sector = NULL;
    dir_entry_t* entry = fat16_find_in_directory(name_83, &dir_sector);
    if (!entry) return -1; // File not found

    // Free the cluster chain associated with the file
    fat16_free_cluster_chain(entry->start_cluster);

    // Mark the entry as deleted
    entry->name[0] = (char)0xE5; // Mark as deleted

    // Write the updated directory sector back to disk
    if (update_directory(dir_sector))
        return -1; // Failed to update directory

    return 0;
}

int fat16_create_dir(const char* name) {
    uint16_t start_cluster = fat16_alloc_cluster();
    if (start_cluster == 0) return -1; // No free clusters available

    // Create the new directory entry
    if (fat16_create_entry(name, ATTR_DIRECTORY, start_cluster, 0) != 0)
        return -1; // Failed to create directory entry

    // Initialize the new directory with a "." and ".." entry
    dir_entry_t* new_dir_entries = kmalloc(SECTOR_SIZE);
    memset(new_dir_entries, 0, SECTOR_SIZE);

    strncpy(new_dir_entries[0].name, ".          ", 11);
    new_dir_entries[0].attr = ATTR_DIRECTORY;
    new_dir_entries[0].start_cluster = start_cluster;
    new_dir_entries[0].file_size = 0;

    strncpy(new_dir_entries[1].name, "..         ", 11);
    new_dir_entries[1].attr = ATTR_DIRECTORY;
    new_dir_entries[1].start_cluster = open_dir ? open_dir->cluster : 0; // Parent directory cluster
    new_dir_entries[1].file_size = 0;

    if (fat16_write_cluster_chain(start_cluster, new_dir_entries, 1)) {
        kfree(new_dir_entries);
        return -1; // Failed to write new directory
    }

    kfree(new_dir_entries);
    fat16_flush();
    
    return 0; // Directory created successfully
}

void fat16_list_dir() {
    char name[12];
    dir_t* dir = open_dir;

    // Print the entries in the directory
    while (dir) {
        for (int i = 0; i < ENTRIES_PER_SECTOR; i++) {
            if (dir->entries[i].name[0] == 0 || dir->entries[i].name[0] == (char)0xE5)
                continue; // Skip empty or deleted entries

            memcpy(name, dir->entries[i].name, 11);
            name[11] = '\0'; // Null-terminate the name
            printf("%s", name);
            printf(" %s\n", (dir->entries[i].attr & ATTR_DIRECTORY) ? "<DIR>" : "");
        }
        dir = dir->next;
    }
}

int fat16_change_directory(const char* path) {
    if (!path || strlen(path) == 0) return -1; // Invalid path

    // Parse the path
    path_t* parsed_path = parse_path(path);
    if (!parsed_path) return -1; // Failed to parse path

    // Open the directory
    int result = fat16_open_directory(parsed_path);
    destroy_path(parsed_path); // Free the parsed path structure

    if (result != 0)
        return -1; // Failed to open directory

    return 0; // Success
}

int fat16_create_entry(const char* name, uint8_t attr, uint16_t start_cluster, uint32_t size) {
    dir_t* dir = open_dir;
    // Find a free entry in the directory
    while (1) {
        for (int i = 0; i < ENTRIES_PER_SECTOR; i++) {
            if (dir->entries[i].name[0] == 0 || dir->entries[i].name[0] == (char)0xE5) {
                memset(&dir->entries[i], 0, sizeof(dir_entry_t));
                strncpy(dir->entries[i].name, format_filename_83(name), 11);
                dir->entries[i].attr = attr;
                dir->entries[i].start_cluster = start_cluster;
                dir->entries[i].file_size = size;
                // Write the updated directory sector back to disk
                ide_write_sector(cluster_to_lba(dir->cluster), (uint8_t*)dir);
                return 0;
            }
        }
        if (!dir->next)
            break; // No more sectors in the directory chain

        dir = dir->next;
    }

    // No free entry found allocate a new cluster for the directory
    uint16_t new_cluster = fat16_alloc_cluster();
    if (new_cluster == 0)
        return -1; // No free clusters available

    // Create a new directory entry
    dir_entry_t new_entry = {
        .name = {0},
        .attr = attr,
        .start_cluster = new_cluster,
        .file_size = size
    };

    strncpy(new_entry.name, format_filename_83(name), 11);
    dir_t* new_dir = kmalloc(sizeof(dir_t));
    if (!new_dir)
        return -1; // Memory allocation failed
    dir->next = new_dir;

    fat16_set_fat_entry(dir->cluster, new_cluster);

    new_dir->cluster = new_cluster;
    new_dir->next = NULL;
    memset(new_dir->entries, 0, SECTOR_SIZE);
    new_dir->entries[0] = new_entry;

    // Write the new directory entry to the FAT
    fat16_set_fat_entry(new_cluster, FAT16_CLUSTER_EOF); // Mark the end of the chain
    ide_write_sector(cluster_to_lba(new_cluster), (uint8_t*)new_dir->entries);
    fat16_flush();

    return 0;
}

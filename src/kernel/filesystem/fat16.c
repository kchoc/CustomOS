#include "kernel/filesystem/fat16.h"
#include "kernel/filesystem/vfs.h"
#include "kernel/filesystem/path.h"
#include "kernel/filesystem/file.h"

#include "kernel/drivers/ide.h"

#include "kernel/memory/kmalloc.h"
#include "kernel/terminal.h"
#include "kernel/types.h"

#include "types/string.h"

#define ENTRY_FREE(entry) ((entry)->name[0] == 0 || (entry)->name[0] == (char)0xE5)

// FAT16 Filesystem Implementation

inode_ops_t fat16_inode_ops = {
    .lookup = fat16_lookup,
    .create = fat16_create,
    .link = fat16_link,
    .unlink = fat16_unlink,
    .mkdir  = fat16_mkdir,
    .rmdir  = fat16_rmdir,
    .rename = fat16_rename
};

file_ops_t fat16_file_ops = {
    .llseek   = fat16_llseek,
    .read    = fat16_read,
    .write   = fat16_write,
    .open    = fat16_open,
    .release = fat16_release,
    .iterate_shared = fat16_iterate_shared
};

static inline uint32_t cluster_to_lba(uint16_t cluster) {
    return DATA_CLUSTER_OFFSET + cluster - 2;
}

dentry_t* fat16_mount(file_system_type_t* fs_type, int flags,
                        block_device_t* device, void* data) {
    if (!fs_type || strcmp(fs_type->name, "fat16") != 0) return NULL;

    fat16_init();

    super_block_t* sb = alloc_superblock(fs_type, NULL);
    if (!sb) return NULL;
    sb->s_magic = 0x4D44; // 'DM' signature for FAT16
    sb->block_size = SECTOR_SIZE;
    sb->s_op = NULL; // Not used in this simple implementation
    sb->device = device;

    inode_t* root_inode = alloc_inode(0, UMODE_IFDIR, 0, &fat16_inode_ops, &fat16_file_ops); // Directory mode
    if (!root_inode) goto cleanup_sb;
    root_inode->i_sb = sb;

    fat16_node_info_t* root_info = alloc_fat16_node_info(0, 0, 0, 0);
    if (!root_info) goto cleanup_root_inode;
    root_inode->private = root_info;

    dentry_t* root_dentry = alloc_dentry("/", root_inode, NULL);
    if (!root_dentry) goto cleanup_root_info;
    root_dentry->d_sb = sb;

    sb->s_root = root_dentry;

    return root_dentry;

    cleanup_root_inode:
    kfree(root_inode);
    cleanup_root_info:
    kfree(root_info);
    cleanup_sb:
    kfree(sb);
    return NULL;
}

/* --------------------
   FAT16 LOOKUP
   -------------------- */

/* ---- Lookup Context and Callbacks ---- */

typedef struct {
    uint32_t lba;
    uint16_t index;
    char name[11];
    fat16_dir_entry_t* found;
} lookup_ctx_t;

static bool fat16_lookup_cb(fat16_dir_entry_t* entry, void* ctx, uint16_t lba, uint16_t index) {
    if (ENTRY_FREE(entry)) return true;

    lookup_ctx_t* lctx = (lookup_ctx_t*)ctx;
    if (strncmp(entry->name, lctx->name, 11) == 0) {
        memcpy(lctx->found, entry, sizeof(fat16_dir_entry_t));
        lctx->lba = lba;
        lctx->index = index;
        return false; // stop
    }
    return true; // continue
}

typedef struct {
    uint16_t cluster;
    uint32_t lba;
    uint16_t index;
    fat16_dir_entry_t* found;
} lookup_cluster_ctx_t;

static bool fat16_lookup_cluster_cb(fat16_dir_entry_t* entry, void* ctx, uint16_t lba, uint16_t index) {
    if (ENTRY_FREE(entry)) return true;

    lookup_cluster_ctx_t* lctx = (lookup_cluster_ctx_t*)ctx;
    if (entry->start_cluster == lctx->cluster) {
        memcpy(lctx->found, entry, sizeof(fat16_dir_entry_t));
        lctx->lba = lba;
        lctx->index = index;
        return false; // stop
    }
    return true; // continue
}

typedef struct {
    uint32_t lba;
    uint16_t index;
} empty_ctx_t;

static bool fat16_find_empty_cb(fat16_dir_entry_t* entry, void* ctx, uint16_t lba, uint16_t index) {
    if (!ENTRY_FREE(entry)) return true;

    empty_ctx_t* ectx = (empty_ctx_t*)ctx;
    ectx->lba = lba;
    ectx->index = index;
    return false; // Found free entry, stop iteration
}

typedef struct {
    uint32_t lba;
    uint16_t index;
    dir_context_t *dir;
} readdir_ctx_t;

static bool fat16_readdir_cb(fat16_dir_entry_t* entry, void* ctx, uint16_t lba, uint16_t index) {
    if (entry->name[0] == 0)
        return false; // Stop on empty entry
    if (entry->name[0] == (char)0xE5)
        return true; // Skip deleted entry

    readdir_ctx_t* rctx = (readdir_ctx_t*)ctx;
    if (!rctx->dir->actor(rctx->dir, entry->name, 11, entry->start_cluster, entry->file_size, 0))
        return false; // Stop if callback returns false

    return true; // Continue
}

static bool fat16_check_empty_dir_cb(fat16_dir_entry_t* entry, void* ctx, uint16_t lba, uint16_t index) {
    if (!ENTRY_FREE(entry) && (entry->name[0] != '.')) {
        bool* is_empty = (bool*)ctx;
        *is_empty = false; // Found a non-empty entry
        return false; // Stop iteration
    }
    return true; // Continue
}

typedef bool (*fat16_entry_cb)(fat16_dir_entry_t* entry, void* context, uint16_t lba, uint16_t index);

/* ---- Directory Iteration ---- */

#define MAX_ITTERATIONS 1024 // Prevent infinite loops incase of corruption

static bool fat16_iterate_dir(uint16_t cluster, fat16_entry_cb cb, void* context) {
    KMALLOC_RET(buf, void, SECTOR_SIZE, false);
    fat16_dir_entry_t* entries = (fat16_dir_entry_t*)buf;
    int itter = 0;
    while (cluster < FAT16_CLUSTER_EOF && itter++ < MAX_ITTERATIONS) {
        uint32_t lba = cluster_to_lba(cluster);
        ide_read_sector(lba, buf);

        for (int i = 0; i < ENTRIES_PER_SECTOR; i++) {
            if (!cb(&entries[i], context, lba, i))
                return true; // Callback signaled to stop iteration
        }
        cluster = fat16_get_next_cluster(cluster);
    }
    return false; // Completed iteration without callback signaling to stop
}

static bool fat16_iterate_root(fat16_entry_cb cb, void* context) {
    KMALLOC_RET(buf, void, SECTOR_SIZE, false);
    fat16_dir_entry_t* entries = (fat16_dir_entry_t*)buf;
    uint32_t lba;
    for (uint16_t sector = 0; sector < ROOT_DIR_SECTORS; sector++) {
        lba = ROOT_DIR_LBA + sector;
        ide_read_sector(lba, buf);

        for (int i = 0; i < ENTRIES_PER_SECTOR; i++) {
            if (!cb(&entries[i], context, lba, i))
                return true; // Callback signaled to stop iteration
        }
    }
    return false; // Completed iteration without callback signaling to stop
}

static bool fat16_iterate(inode_t* inode, fat16_entry_cb cb, void* context) {
    if (inode->i_ino == 0)
        return fat16_iterate_root(cb, context);
    else
        return fat16_iterate_dir(inode->i_ino, cb, context);
}

/* --------------------
   FAT16 DIRECTORY OPERATIONS
   -------------------- */

static void fat16_dir_update_entry(uint32_t lba, uint16_t index, uint32_t file_size) {
    KMALLOC_RET(buf, void, SECTOR_SIZE, );
    ide_read_sector(lba, buf);
    fat16_dir_entry_t* entries = (fat16_dir_entry_t*)buf;

    entries[index].file_size = file_size;

    ide_write_sector(lba, buf);
}

static void fat16_dir_add_entry(uint32_t lba, uint16_t index, fat16_dir_entry_t* new_entry) {
    KMALLOC_RET(buf, void, SECTOR_SIZE, );
    ide_read_sector(lba, buf);
    fat16_dir_entry_t* entries = (fat16_dir_entry_t*)buf;

    memcpy(&entries[index], new_entry, sizeof(fat16_dir_entry_t));

    ide_write_sector(lba, buf);
}

static void fat16_dir_remove_entry(uint32_t lba, uint16_t index) {
    KMALLOC_RET(buf, void, SECTOR_SIZE, );
    ide_read_sector(lba, buf);
    fat16_dir_entry_t* entries = (fat16_dir_entry_t*)buf;

    entries[index].name[0] = 0xE5; // Mark as deleted

    ide_write_sector(lba, buf);
}

/* ----------------------
   FAT16 FILE OPERATIONS
   ---------------------- */

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

loff_t fat16_llseek(file_t* file, loff_t offset, int whence) {
    if (!file || !file->private) return -1; // Invalid parameters

    fat16_file_t* fat_file = file->private;
    loff_t new_pos;

    switch (whence) {
        case SEEK_SET: new_pos = offset; break;
        case SEEK_CUR: new_pos = file->f_pos + offset; break;
        case SEEK_END: new_pos = fat_file->file_size + offset; break;
        default: return -1; // Invalid whence
    }

    if (new_pos < 0 || new_pos > fat_file->file_size) return -1;

    // Walk cluster chain to get cluster for new_pos
    uint16_t cluster = fat_file->start_cluster;
    loff_t pos = 0;
    while (cluster != FAT16_CLUSTER_EOF && pos + SECTOR_SIZE <= new_pos) {
        cluster = fat16_get_next_cluster(cluster);
        pos += SECTOR_SIZE;
    }

    fat_file->current_cluster = cluster;
    file->f_pos = new_pos;
    return new_pos;
}

ssize_t fat16_read(file_t* file, char* __user buf, size_t count, loff_t* offset) {
    if (!file || !file->private || !buf) return -1; // Invalid parameters
    if (offset) *offset = file->f_pos;

    KMALLOC_RET(cluster_buf, void, SECTOR_SIZE, -1);

    fat16_file_t* fat_file = file->private;
    uint32_t cluster_offset = file->f_pos % SECTOR_SIZE;
    uint16_t current_cluster = fat_file->current_cluster;

    size_t to_read = count;
    if (file->f_pos + to_read > fat_file->file_size)
        to_read = fat_file->file_size - file->f_pos; // Adjust to not read past EOF

    size_t total_read = 0;
    size_t read_now = 0;

    while (to_read > 0 && current_cluster < FAT16_CLUSTER_EOF) {
        if (cluster_offset == 0 && file->f_pos != 0) {
            current_cluster = fat16_get_next_cluster(current_cluster);
            fat_file->current_cluster = current_cluster;
        }

        ide_read_sector(cluster_to_lba(current_cluster), cluster_buf);
        read_now = SECTOR_SIZE - cluster_offset;
        if (read_now > to_read) read_now = to_read;
        memcpy((uint8_t*)buf + total_read, (uint8_t*)cluster_buf + cluster_offset, read_now);
        total_read += read_now;
        to_read -= read_now;
        cluster_offset = file->f_pos % SECTOR_SIZE;
    }

    file->f_pos += total_read;

    return total_read; // Number of bytes read
}

ssize_t fat16_write(file_t* file, const char* __user buf, size_t count, loff_t* offset) {
    if (!file || !file->private || !buf) return -1; // Invalid parameters
    if (offset) *offset = file->f_pos;

    KMALLOC_RET(cluster_buf, void, SECTOR_SIZE, -1);

    fat16_file_t* fat_file = file->private;
    uint32_t cluster_offset = file->f_pos % SECTOR_SIZE;
    uint16_t current_cluster = fat_file->current_cluster;
    uint16_t next_cluster = fat16_get_next_cluster(current_cluster);

    size_t to_write = count;
    size_t total_written = 0;
    size_t write_now = 0;

    while (to_write > 0) {
        if (cluster_offset == 0 && file->f_pos != 0) {
            if (next_cluster >= FAT16_CLUSTER_EOF) {
                // Allocate new cluster
                uint16_t new_cluster = fat16_alloc_cluster();
                if (new_cluster == 0) break; // No space left
                fat16_set_next_cluster(current_cluster, new_cluster);
                fat16_set_next_cluster(new_cluster, FAT16_CLUSTER_EOF);
                next_cluster = new_cluster;
            }
            current_cluster = next_cluster;
            next_cluster = fat16_get_next_cluster(current_cluster);
            fat_file->current_cluster = current_cluster;
        }


        ide_read_sector(cluster_to_lba(current_cluster), cluster_buf);
        write_now = SECTOR_SIZE - cluster_offset;
        if (write_now > to_write) write_now = to_write;
        memcpy((uint8_t*)cluster_buf + cluster_offset, (uint8_t*)buf + total_written, write_now);
        ide_write_sector(cluster_to_lba(current_cluster), cluster_buf);
        total_written += write_now;
        to_write -= write_now;
        cluster_offset = file->f_pos % SECTOR_SIZE;
    }

    file->f_pos += total_written;
    if (file->f_pos > fat_file->file_size)
        fat_file->file_size = file->f_pos;

    fat16_flush();

    return total_written; // Number of bytes written
}

int fat16_open(inode_t* inode, file_t* file) {
    if (!inode || !file) return -1; // Invalid parameters

    fat16_file_t* fdata = kmalloc(sizeof(fat16_file_t));
    if (!fdata) return -1; // Memory allocation failure

    dentry_t* dentry = file->f_dentry;
    if (!dentry || !dentry->d_inode || !dentry->d_inode->private) {
        kfree(fdata);
        return -1; // Invalid dentry or inode
    }

    fat16_node_info_t* node_info = (fat16_node_info_t*)dentry->d_inode->private;
    fdata->file_size = node_info->file_size;
    fdata->start_cluster = node_info->start_cluster;
    fdata->current_cluster = node_info->start_cluster;
    fdata->parent_dir_lba = node_info->dir_lba;
    fdata->parent_dir_index = node_info->dir_index;
    file->private = fdata;

    return 0; // Success
}

int fat16_release(inode_t* inode, file_t* file) {
    if (!inode || !file || !file->private) return -1; // Invalid parameters

    fat16_file_t* fdata = file->private;
    fat16_node_info_t* node_info = (fat16_node_info_t*)inode->private;
    if (fdata && node_info) {
        // Update inode info
        node_info->file_size = fdata->file_size;
        node_info->start_cluster = fdata->start_cluster;

        // Update directory entry on disk
        fat16_dir_update_entry(fdata->parent_dir_lba, fdata->parent_dir_index, fdata->file_size);
    }

    kfree(fdata);
    file->private = NULL;

    return 0; // Success
}

int fat16_iterate_shared(file_t* file, dir_context_t* ctx) {
    if (!file || !file->private || !ctx) return -1; // Invalid parameters

    fat16_file_t* fdata = file->private;
    if (fdata->start_cluster == 0) {
        // Root directory
        readdir_ctx_t rctx = { .lba = 0, .index = 0, .dir = ctx };
        fat16_iterate_root(fat16_readdir_cb, &rctx);
    } else {
        // Subdirectory
        readdir_ctx_t rctx = { .lba = 0, .index = 0, .dir = ctx };
        fat16_iterate_dir(fdata->start_cluster, fat16_readdir_cb, &rctx);
    }

    return 0; // Success
}

/* ----------------------
   FAT16 INODE OPERATIONS
   ---------------------- */

fat16_node_info_t* alloc_fat16_node_info(uint16_t start_cluster, uint32_t file_size, uint32_t dir_lba, uint16_t dir_index) {
    fat16_node_info_t* node_info = kmalloc(sizeof(fat16_node_info_t));
    if (!node_info) return NULL;

    node_info->start_cluster = start_cluster;
    node_info->file_size = file_size;
    node_info->dir_lba = dir_lba;
    node_info->dir_index = dir_index;

    return node_info;
}
 
dentry_t *fat16_lookup(inode_t* dir, const char* name, unsigned int flags) {
    if (!dir || !name) return NULL; // Invalid parameters

    fat16_dir_entry_t found_entry = {0};
    lookup_ctx_t ctx = { .name = 0, .found = &found_entry };
    if (format_filename_83(name, (char*)ctx.name)) return NULL;

    fat16_iterate(dir, fat16_lookup_cb, &ctx);

    if (found_entry.name[0] == 0) return NULL; // Entry not found

    // Create new inode and dentry
    inode_t* new_inode = alloc_inode(found_entry.start_cluster,
                                   (found_entry.attr & ATTR_DIRECTORY) ? 0x4000 : 0x8000,
                                   found_entry.file_size,
                                   &fat16_inode_ops,
                                   &fat16_file_ops);
    if (!new_inode) return NULL;
    
    fat16_node_info_t* node_info = kmalloc(sizeof(fat16_node_info_t));
    if (!node_info) goto cleanup_inode;

    node_info->start_cluster = found_entry.start_cluster;
    node_info->file_size = found_entry.file_size;
    node_info->dir_lba = ctx.lba;
    node_info->dir_index = ctx.index;
    new_inode->private = node_info;
    new_inode->i_sb = dir->i_sb;
    dentry_t* new_dentry = alloc_dentry(name, new_inode, NULL);
    if (!new_dentry) goto cleanup_inode_info;

    return new_dentry;

    cleanup_inode_info:
    kfree(node_info);
    cleanup_inode:
    kfree(new_inode);
    return NULL;
}

int fat16_create(inode_t* dir, dentry_t* dentry, umode_t mode, bool excl) {
    if (!dir || !dentry || !dentry->d_name) return -1;

    char name_83[11];
    if (format_filename_83(dentry->d_name, name_83)) return -1; // invalid name

    // Check if file already exists
    fat16_dir_entry_t entry = {0};
    lookup_ctx_t ctx = { .found = &entry };
    strncpy(ctx.name, name_83, 11);
    fat16_iterate(dir, fat16_lookup_cb, &ctx);
    if (entry.name[0] != 0) {
        if (excl) return -1; // file exists & exclusive flag set
        // otherwise overwrite 
        // TODO: handle truncation
    }

    // Find an empty entry
    empty_ctx_t empty_ctx = { .lba = 0xFFFFFFFF, .index = 0xFFFF };
    if (!fat16_iterate(dir, fat16_find_empty_cb, &empty_ctx)) return -1;

    if (empty_ctx.lba == 0xFFFFFFFF) return -1; // No free entry found

    uint16_t new_cluster = fat16_alloc_cluster();

    // Create new FAT entry
    entry = (fat16_dir_entry_t){ .attr = ATTR_ARCHIVE, .start_cluster = new_cluster, .file_size = 0 };
    strncpy(entry.name, name_83, 11);

    // Write directory entry
    fat16_dir_add_entry(empty_ctx.lba, empty_ctx.index, &entry);

    // Create inode for new file
    inode_t* new_inode = alloc_inode(0, 0x8000, 0, &fat16_inode_ops, &fat16_file_ops);
    if (!new_inode) return -1;

    fat16_node_info_t* node_info = alloc_fat16_node_info(0, 0, empty_ctx.lba, empty_ctx.index);
    if (!node_info) {
        kfree(new_inode);
        return -1;
    }

    new_inode->private = node_info;
    new_inode->i_sb = dir->i_sb;

    dentry->d_inode = new_inode;

    fat16_flush();
    return 0;
}

int fat16_link(inode_t* dir, dentry_t* old_dentry, dentry_t* new_dentry) {
    if (!dir || !old_dentry || !new_dentry || !old_dentry->d_inode || !old_dentry->d_inode->private || !new_dentry->d_name)
        return -1; // Invalid parameters

    char name_83[11];
    if (format_filename_83(new_dentry->d_name, name_83)) return -1; // Invalid filename format

    // Check if new entry already exists
    fat16_dir_entry_t entry = {0};
    lookup_ctx_t ctx = { .name = 0, .found = &entry };
    strncpy(ctx.name, name_83, 11);
    if (fat16_iterate(dir, fat16_lookup_cb, &ctx)) return -1; // Entry already exists

    // Find an empty entry
    empty_ctx_t empty_ctx = { .lba = 0xFFFFFFFF, .index = 0xFFFF };
    if (!fat16_iterate(dir, fat16_find_empty_cb, &empty_ctx)) return -1; // No free entry found
    if (empty_ctx.lba == 0xFFFFFFFF) return -1; // No free entry found

    fat16_node_info_t* old_node_info = (fat16_node_info_t*)old_dentry->d_inode->private;
    if (!old_node_info) return -1; // Invalid old inode info

    // Create new directory entry pointing to the same cluster
    entry = (fat16_dir_entry_t){0};
    strncpy(entry.name, name_83, 11);
    entry.attr = (old_dentry->d_inode->i_mode == 0x4000) ? ATTR_DIRECTORY : ATTR_ARCHIVE;
    entry.start_cluster = old_node_info->start_cluster;
    entry.file_size = old_node_info->file_size;

    // Write the new entry to disk
    fat16_dir_add_entry(empty_ctx.lba, empty_ctx.index, &entry);

    // Create inode for new link
    inode_t* new_inode = alloc_inode(old_node_info->start_cluster,
                                   old_dentry->d_inode->i_mode,
                                   old_node_info->file_size,
                                   &fat16_inode_ops,
                                   &fat16_file_ops);
    if (!new_inode) return -1;

    fat16_node_info_t* node_info = alloc_fat16_node_info(old_node_info->start_cluster,
                                                        old_node_info->file_size,
                                                        empty_ctx.lba, 
                                                        empty_ctx.index);
    if (!node_info) {
        kfree(new_inode);
        return -1;
    }
    new_inode->private = node_info;
    new_inode->i_sb = dir->i_sb;
    new_dentry->d_inode = new_inode;
    fat16_flush(); // Flush FAT changes to disk
    return 0; // Success
}

int fat16_unlink(inode_t* inode, dentry_t* dentry) {
    if (!inode || !(inode->i_mode & 0x4000) || !dentry || !dentry->d_inode || !dentry->d_inode->private)
        return -1; // Invalid parameters

    fat16_node_info_t* node_info = (fat16_node_info_t*)dentry->d_inode->private;
    if (!node_info) return -1; // Invalid inode info

    // Free the cluster chain
    if (node_info->start_cluster != 0)
        fat16_free_cluster_chain(node_info->start_cluster);

    // Remove directory entry
    fat16_dir_remove_entry(node_info->dir_lba, node_info->dir_index);

    // Free inode and its private data
    kfree(node_info);
    kfree(dentry->d_inode);
    dentry->d_inode = NULL;

    fat16_flush(); // Flush FAT changes to disk
    return 0; // Success
}

int fat16_mkdir(inode_t* inode, dentry_t* dentry, umode_t mode) {
    if (!inode || !(inode->i_mode & 0x4000)  || !dentry || !dentry->d_name) return -1; // Invalid parameters

    char name_83[11];
    if (format_filename_83(dentry->d_name, name_83)) return -1; // Invalid filename format

    // Check if directory already exists
    fat16_dir_entry_t entry = {0};
    lookup_ctx_t ctx = { .name = 0, .found = &entry };
    if (fat16_iterate(inode, fat16_lookup_cb, &ctx)) return -1;

    // Find an empty entry
    empty_ctx_t empty_ctx = { .lba = 0xFFFFFFFF, .index = 0xFFFF };
    if (!fat16_iterate(inode, fat16_find_empty_cb, &empty_ctx)) return -1; // No free entry found
    if (empty_ctx.lba == 0xFFFFFFFF) return -1; // No free entry found

    // KMALLOC buffer before allocating a new cluster for easier error handling
    KMALLOC_RET(buf, void, SECTOR_SIZE, -1);

    // Allocate a new cluster for the directory
    uint16_t new_cluster = fat16_alloc_cluster();
    if (new_cluster == 0) return -1; // No free clusters available

    // Create new directory entry
    entry = (fat16_dir_entry_t){0};
    strncpy(entry.name, name_83, 11);
    entry.attr = ATTR_DIRECTORY;
    entry.start_cluster = new_cluster;
    entry.file_size = 0;

    // Write the new entry to disk
    fat16_dir_add_entry(empty_ctx.lba, empty_ctx.index, &entry);

    // Initialize the new directory cluster with '.' and '..' entries
    memset(buf, 0, SECTOR_SIZE);

    fat16_dir_entry_t* entries = (fat16_dir_entry_t*)buf;

    // '.' entry
    strncpy(entries[0].name, ".          ", 11);
    entries[0].attr = ATTR_DIRECTORY;
    entries[0].start_cluster = new_cluster;
    entries[0].file_size = 0;

    // '..' entry
    strncpy(entries[1].name, "..         ", 11);
    entries[1].attr = ATTR_DIRECTORY;
    entries[1].start_cluster = inode->i_ino; // Parent directory cluster
    entries[1].file_size = 0;

    ide_write_sector(cluster_to_lba(new_cluster), buf);

    fat16_flush(); // Flush FAT changes to disk

    return 0; // Success
}

int fat16_rmdir(inode_t* inode, dentry_t* dentry) {
    if (!inode || !(inode->i_mode & 0x4000) || !dentry || !dentry->d_inode || !dentry->d_inode->private)
        return -1; // Invalid parameters

    fat16_node_info_t* node_info = (fat16_node_info_t*)dentry->d_inode->private;
    if (!node_info) return -1; // Invalid inode info

    // Check if directory is empty (only '.' and '..' allowed)
    bool is_empty = true;
    fat16_iterate_dir(node_info->start_cluster, fat16_check_empty_dir_cb, &is_empty);
    if (!is_empty) return -1; // Directory not empty

    // Free the cluster chain
    if (node_info->start_cluster != 0)
        fat16_free_cluster_chain(node_info->start_cluster);

    // Remove directory entry
    fat16_dir_remove_entry(node_info->dir_lba, node_info->dir_index);

    // Free inode and its private data
    kfree(node_info);
    kfree(dentry->d_inode);
    dentry->d_inode = NULL;

    fat16_flush(); // Flush FAT changes to disk
    return 0; // Success
}

int fat16_rename(inode_t* old_dir, dentry_t* old_dentry, inode_t* new_dir, dentry_t* new_dentry, unsigned int flags) {
    if (!old_dir || !(old_dir->i_mode & 0x4000) || !new_dir || !(new_dir->i_mode & 0x4000) ||
        !old_dentry || !old_dentry->d_inode || !old_dentry->d_inode->private ||
        !new_dentry || !new_dentry->d_name) {
        return -1; // Invalid parameters
    }

    char new_name_83[11];
    if (format_filename_83(new_dentry->d_name, new_name_83)) return -1; // Invalid filename format

    // Check if new entry already exists
    fat16_dir_entry_t entry = {0};
    lookup_ctx_t ctx = { .name = 0, .found = &entry };
    strncpy(ctx.name, new_name_83, 11);
    if (fat16_iterate(new_dir, fat16_lookup_cb, &ctx)) return -1; // Entry already exists

    // Find an empty entry in the new directory
    empty_ctx_t empty_ctx = { .lba = 0xFFFFFFFF, .index = 0xFFFF };
    if (!fat16_iterate(new_dir, fat16_find_empty_cb, &empty_ctx)) return -1; // No free entry found
    if (empty_ctx.lba == 0xFFFFFFFF) return -1; // No free entry found

    fat16_node_info_t* old_node_info = (fat16_node_info_t*)old_dentry->d_inode->private;
    if (!old_node_info) return -1; // Invalid old inode info

    // Create new directory entry in the new directory
    entry = (fat16_dir_entry_t){0};
    strncpy(entry.name, new_name_83, 11);
    entry.attr = (old_dentry->d_inode->i_mode == 0x4000) ? ATTR_DIRECTORY : ATTR_ARCHIVE;
    entry.start_cluster = old_node_info->start_cluster;
    entry.file_size = old_node_info->file_size;

    // Write the new entry to disk
    fat16_dir_add_entry(empty_ctx.lba, empty_ctx.index, &entry);

    // Remove the old directory entry
    fat16_dir_remove_entry(old_node_info->dir_lba, old_node_info->dir_index);
    fat16_flush(); // Flush FAT changes to disk
    // Update inode info
    old_node_info->dir_lba = empty_ctx.lba;
    old_node_info->dir_index = empty_ctx.index;
    new_dentry->d_inode = old_dentry->d_inode;
    old_dentry->d_inode = NULL;
    return 0; // Success
}

// OLD FAT16 IMPLEMENTATION

//TODO:
// Add checkign to exclude filenames starting with : (for root directory)

static uint8_t fat[FAT_SECTORS * SECTOR_SIZE];  // 512B * 32 = 16KB
static bpb_t* bpb;

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

void fat16_set_next_cluster(uint16_t cluster, uint16_t next) {
    ((uint16_t*)fat)[cluster] = next;
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

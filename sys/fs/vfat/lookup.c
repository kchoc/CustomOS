#include "lookup.h"
#include "mount.h"
#include "vnode.h"

#include <fs/block.h>

#include <sys/device.h>

#include <kern/terminal.h>
#include <kern/errno.h>

#include <string.h>

uint64_t cluster_to_lba(vfat_mount_data_t* mount_data, uint16_t cluster) {
  return ((uint64_t)(mount_data->data_start_sector + (cluster - 2) * mount_data->bpb.sectors_per_cluster));
}

static inline uint64_t cluster_to_fat_offset(vfat_mount_data_t* mount_data, uint32_t cluster) {
  uint32_t fat_entry_size = (mount_data->fat_type == FAT_TYPE_32) ? 4 : ((mount_data->fat_type == FAT_TYPE_16) ? 2 : 1);
  return mount_data->fat_start_sector * mount_data->bpb.bytes_per_sector + cluster * fat_entry_size;
}

int follow_cluster_chain(device_t* dev, vfat_mount_data_t* mount_data, uint32_t start_cluster, uint32_t* next_cluster) {
  uint64_t fat_offset = cluster_to_fat_offset(mount_data, start_cluster);
  uint8_t buffer[4]; // Max size needed for FAT32 entry
  void* block_buffer;
  int res = block_read(dev, fat_offset / mount_data->bpb.bytes_per_sector, &block_buffer, mount_data->bpb.bytes_per_sector); 
  if (res) return res;
  memcpy(buffer, (uint8_t*)block_buffer + (fat_offset % mount_data->bpb.bytes_per_sector), 4);
  block_release(block_buffer);
  *next_cluster = (mount_data->fat_type == FAT_TYPE_32) ? (buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | ((buffer[3] & 0x0F) << 24)) :
                   ((mount_data->fat_type == FAT_TYPE_16) ? (buffer[0] | (buffer[1] << 8)) : (buffer[0] | ((buffer[1] & 0x0F) << 8)));
  return 0;
}

static inline uint8_t lfn_check_checksum(const uint8_t* short_name) {
  uint8_t checksum = 0;
  for (int i = 0; i < 11; i++) {
    checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + short_name[i];
  }
  return checksum;
}

static void vfat_utf16_to_ascii(char *out, uint16_t *in, size_t len) {
  size_t j = 0;

  for (size_t i = 0; i < len; i++) {
    uint16_t c = in[i];

    if (c == 0x0000) break;
    if (c == 0xFFFF) continue;

    if (c < 0x80)
      out[j++] = (char)c;
    else
      out[j++] = '?';
  }

  out[j] = '\0';
}

void vfat_build_shortname(char *out, const uint8_t name[11]) {
  int i, j = 0;

  // base
  for (i = 0; i < 8 && name[i] != ' '; i++)
    out[j++] = name[i];

  // extension
  if (name[8] != ' ') {
    out[j++] = '.';
    for (i = 8; i < 11 && name[i] != ' '; i++)
      out[j++] = name[i];
  }

  out[j] = '\0';
}

int vfat_lookup(struct vnode *dvp, const char *name, int flags, vfat_lookup_result_t *out) {
    if (!dvp || !name || !out) return -EINVAL;
    if (strlen(name) > 255) return -ENAMETOOLONG;

    vfat_node_data_t *dir = dvp->v_data;
    vfat_mount_data_t *mnt = dvp->v_mount->private;
    vnode_t *dev = dvp->v_mount->mnt_dev_vnode;

    bool use_lfn = (mnt->flags & VFAT_FLAG_LFN_READ);

    memset(out, 0, sizeof(*out)); 

    uint32_t next_cluster = dir->start_cluster;
    uint32_t cluster = dir->start_cluster;
    uint32_t sector;
    void *buffer;

    bool is_valid_lfn = false;
    uint32_t lfn_checksum = 0;
    uint8_t prev_lfn_order = 0;
    size_t lfn_index = 0;

    bool free_found = false;
    uint8_t free_count = 0;
    uint32_t free_cluster_start;
    size_t free_cluster_offset;
    uint32_t min_free_entries = (strlen(name) + 12) / 13;

    size_t off = 0;
    size_t bytes = dir->is_root && (mnt->fat_type == FAT_TYPE_12 || mnt->fat_type == FAT_TYPE_16) ? mnt->bpb.bytes_per_sector : mnt->cluster_size_bytes;
    while (1) {
        // --- read sector ---
        off = 0;
        cluster = next_cluster;
        if (dir->is_root && (mnt->fat_type == FAT_TYPE_12 || mnt->fat_type == FAT_TYPE_16)) {

            // Handle end of root directory for FAT12/16
            if (cluster >= mnt->root_dir_sectors) goto end;

            // Read next sector of root directory
            sector = mnt->root_dir_start_sector + cluster;
            block_read(dev->v_data, sector, &buffer, mnt->bpb.bytes_per_sector);
            next_cluster++;

        } else {
            if (IS_CLUSTER_END(mnt, cluster)) break;

            sector = cluster_to_lba(mnt, cluster);
            block_read(dev->v_data, sector, &buffer, mnt->cluster_size_bytes);
            follow_cluster_chain(dev->v_data, mnt, cluster, &next_cluster);
        }

        for (off = 0; off < bytes; off += sizeof(vfat_standard_entry_t)) {
            vfat_standard_entry_t *entry = (void*)((uint8_t*)buffer + off);
            
            if (entry->name[0] == 0x00) goto end;

            // --- free slot tracking ---
            if (entry->name[0] == 0xE5) {
                if (!(flags & VFAT_LOOKUP_FREE) || free_found) continue;
                
                if (free_count == 0) {
                    free_cluster_start = cluster;
                    free_cluster_offset = off;
                }

                free_count++;
                if (free_count == min_free_entries) free_found = true;
                continue;
            }
            
            if (!free_found && (flags & VFAT_LOOKUP_FREE)) {
                free_count = 0;
                free_cluster_start = 0;
                free_cluster_offset = 0;
            }

            // --- LFN ---
            if (entry->attributes == 0x0F) {
                if (!use_lfn) continue;

                vfat_lfn_entry_t *e = (void*)entry;
                int order = e->order & 0x1F;

                if (order == 0 || order > 20) {
                    is_valid_lfn = false;
                    continue;
                }

                if (e->order & 0x40) {
                    is_valid_lfn = true;
                    lfn_checksum = e->checksum;
                    prev_lfn_order = order;

                    out->lfn_start_sector = sector;
                    out->lfn_start_offset = off;
                    out->lfn_count = order;
                    goto check_lfn_name;
                }

                if (!is_valid_lfn) continue;


                if (e->checksum != lfn_checksum) {
                    is_valid_lfn = false;
                    continue;
                }

                if (--prev_lfn_order != order) {
                    is_valid_lfn = false;
                    continue;
                }

  check_lfn_name:
                // Check here whether the LFN data in this string matches the corresponding part of the name we're looking for. If not, we can skip the rest of the LFN entries for this file and
                lfn_index = (order - 1) * 13;
                for (size_t i = 0; i < 13 && name[lfn_index + i]; i++) {
                    char c = name[lfn_index + i];
                    char lc = (c >= 'A' && c <= 'Z') ? c + 32 : c;
                    
                    uint16_t wc;
                    if (i < 5) wc = e->name1[i];
                    else if (i < 11) wc = e->name2[i - 5];
                    else wc = e->name3[i - 11];
                    char lc_wc = (wc >= 'A' && wc <= 'Z') ? wc + 32 : wc;
                    
                    if (lc != lc_wc) {
                        is_valid_lfn = false;
                        break;
                    }
                } 
                continue;
            }

            // --- standard 8.3 entry ---
            if (is_valid_lfn) {

                if (lfn_check_checksum(entry->name) != lfn_checksum) {
                    is_valid_lfn = false;
                    continue;
                }
              
                if (prev_lfn_order != 1) {
                    is_valid_lfn = false;
                    continue;
                }

                out->has_lfn = true;
                goto found;
            }

            char short_name[13];
            vfat_build_shortname(short_name, entry->name);
            short_name[12] = '\0';

            for (size_t i = 0; short_name[i]; i++) {
                char c = short_name[i];
                char lc = (c >= 'A' && c <= 'Z') ? c + 32 : c;
                if (lc != name[i]) goto next_entry;
            }

            
found:
            // --- FOUND ---
            block_release(buffer);
            if (flags & VFAT_LOOKUP_FREE) return -EEXIST; // FIND_FREE: Found an existing entry with the same name

            out->sector = sector;
            out->offset = off;
            out->cluster = entry->first_cluster_low |
                           (entry->first_cluster_high << 16);

            memcpy(&out->entry, entry, sizeof(*entry));

            return 0;
next_entry: continue;
        }

        block_release(buffer);
    }

end:
    block_release(buffer);

    if (flags & VFAT_LOOKUP_FIND) return -ENOENT; // FIND: Reached end of directory without finding the entry

    if (free_found) {
        out->sector = free_cluster_start;
        out->offset = free_cluster_offset;
        return 0; // FIND_FREE: Found enough free entries and name is not present
    }

    // Check if there is enough space for the new entry at the end of the directory (either in current cluster or by allocating a new cluster if needed)
    if (dir->is_root && (mnt->fat_type == FAT_TYPE_12 || mnt->fat_type == FAT_TYPE_16)) {
        if (cluster < mnt->root_dir_sectors || (off + min_free_entries * sizeof(vfat_standard_entry_t) <= mnt->bpb.bytes_per_sector)) {
            out->sector = cluster - 1;
            out->offset = off; 
            return 0; // FIND_FREE: There is space for the new entry in the root directory
        }
        return -ENOSPC; // FIND_FREE: No space for new entry in root directory
    }

    out->sector = cluster;
    out->offset = off;
    return 0; // FIND_FREE: Reached end of directory but there is space for new entry (either in current cluster or by allocating a new cluster)
}

int vfat_free_cluster_chain(device_t* dev, vfat_mount_data_t* mount_data, uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    uint8_t empty_entry[4] = {0}; // Max size needed for FAT32 entry

    while (!IS_CLUSTER_END(mount_data, cluster)) {
        uint64_t fat_offset = cluster_to_fat_offset(mount_data, cluster);
        int res = block_write(dev, fat_offset / mount_data->bpb.bytes_per_sector, empty_entry, mount_data->bpb.bytes_per_sector);
        if (res) return res;

        uint32_t next_cluster;
        res = follow_cluster_chain(dev, mount_data, cluster, &next_cluster);
        if (res) return res;

        cluster = next_cluster;
    }

    return 0;
}

int vfat_allocate_cluster(device_t* dev, vfat_mount_data_t* mount_data, uint32_t prev_cluster, uint32_t* new_cluster) {
    uint8_t buffer[4]; // Max size needed for FAT32 entry
    void* block_buffer;

    for (uint32_t cluster = 2; cluster < mount_data->total_clusters + 2; cluster++) {
        uint64_t fat_offset = cluster_to_fat_offset(mount_data, cluster);
        int res = block_read(dev, fat_offset / mount_data->bpb.bytes_per_sector, &block_buffer, mount_data->bpb.bytes_per_sector);
        if (res) return res;

        memcpy(buffer, (uint8_t*)block_buffer + (fat_offset % mount_data->bpb.bytes_per_sector), 4);
        block_release(block_buffer);

        uint32_t entry_value = (mount_data->fat_type == FAT_TYPE_32) ? (buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | ((buffer[3] & 0x0F) << 24)) :
                               ((mount_data->fat_type == FAT_TYPE_16) ? (buffer[0] | (buffer[1] << 8)) : (buffer[0] | ((buffer[1] & 0x0F) << 8)));

        if (entry_value == 0) {
            // Found a free cluster
            memset(buffer, 0xFF, 4); // Mark as end of chain
            res = block_write(dev, fat_offset / mount_data->bpb.bytes_per_sector, buffer, mount_data->bpb.bytes_per_sector);
            if (res) return res;

            if (prev_cluster != 0) {
                // Update previous cluster to point to the new cluster
                uint64_t prev_fat_offset = cluster_to_fat_offset(mount_data, prev_cluster);
                void* prev_block_buffer;
                res = block_read(dev, prev_fat_offset / mount_data->bpb.bytes_per_sector, &prev_block_buffer, mount_data->bpb.bytes_per_sector);
                if (res) return res;

                uint8_t* prev_entry_ptr = (uint8_t*)prev_block_buffer + (prev_fat_offset % mount_data->bpb.bytes_per_sector);
                if (mount_data->fat_type == FAT_TYPE_32) {
                    prev_entry_ptr[0] = cluster & 0xFF;
                    prev_entry_ptr[1] = (cluster >> 8) & 0xFF;
                    prev_entry_ptr[2] = (cluster >> 16) & 0xFF;
                    prev_entry_ptr[3] = ((cluster >> 24) & 0x0F) | (prev_entry_ptr[3] & 0xF0);
                } else if (mount_data->fat_type == FAT_TYPE_16) {
                    prev_entry_ptr[0] = cluster & 0xFF;
                    prev_entry_ptr[1] = (cluster >> 8) & 0xFF;
                } else {
                    prev_entry_ptr[0] = cluster & 0xFF;
                    prev_entry_ptr[1] = ((cluster >> 8) & 0x0F) | (prev_entry_ptr[1] & 0xF0);
                }
                res = block_write(dev, prev_fat_offset / mount_data->bpb.bytes_per_sector, prev_block_buffer, mount_data->bpb.bytes_per_sector);
                block_release(prev_block_buffer);
                if (res) return res;
            }

            *new_cluster = cluster;
            return 0;
        }
    }

    return -ENOSPC; // No free clusters available
}

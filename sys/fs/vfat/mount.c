#include "mount.h"
#include "vnode.h"

#include <fs/block.h>
#include <fs/mount.h>
#include <fs/vnode.h>

#include <vm/kmalloc.h>

#include <kern/terminal.h>
#include <kern/errno.h>

mount_ops_t vfat_mount_ops = {
  .mount = vfat_mount,
  .unmount = vfat_unmount,
  .get_root = vfat_get_root,
  .sync = vfat_sync,
};

int vfat_mount(mount_t* mnt, const char* options) {
  if (!mnt || !mnt->mnt_dev_vnode) return -EINVAL; // Invalid mount structure or device vnode

  vfat_mount_data_t* mount_data = kmalloc(sizeof(vfat_mount_data_t));
  if (!mount_data) return -ENOMEM;

  bpb_t* bpb = &mount_data->bpb; // Point to the BPB within the mount data structure

  void* buffer; 
  int res = block_read(mnt->mnt_dev_vnode->v_data, 0, &buffer, 512);
  if (res) {
    kfree(mount_data);
    return res; // Failed to read the BPB sector
  }

  memcpy(bpb, buffer, sizeof(bpb_t) + sizeof(bpb_extension_fat32_t)); // Copy the BPB data from the buffer into our mount data structure

  mnt->private = mount_data;

  if (bpb->bytes_per_sector == 0 || bpb->sectors_per_cluster == 0 || bpb->fat_count == 0 || (bpb->total_sectors_short == 0 && bpb->total_sectors_long == 0) || bpb->fat_size_sectors == 0) {
    kfree(mount_data);
    block_release(buffer);
    return -EINVAL; // Invalid BPB data
  }

  // Check media descriptor for valid FAT types (0xF8 for fixed disk, 0xF0 for removable media, 0xF9 for RAM disk)
  if (bpb->media_descriptor != 0xF8 && bpb->media_descriptor != 0xF0 && bpb->media_descriptor != 0xF9) {
    kfree(mount_data);
    block_release(buffer);
    return -EINVAL; // Invalid media descriptor for FAT
  }

  // Determine FAT type based on BPB values
  uint32_t total_sectors = bpb->total_sectors_short ? bpb->total_sectors_short : bpb->total_sectors_long;
  uint32_t data_sectors = total_sectors - (bpb->reserved_sector_count + (bpb->fat_count * bpb->fat_size_sectors) + ((bpb->root_entry_count * 32 + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector));
  uint32_t total_clusters = data_sectors / bpb->sectors_per_cluster;

  if (total_clusters < 4085) {
    mount_data->fat_type = FAT_TYPE_12; // FAT12
  } else if (total_clusters < 65525) {
    mount_data->fat_type = FAT_TYPE_16; // FAT16
  } else {
    mount_data->fat_type = FAT_TYPE_32; // FAT32
    mount_data->root_cluster = mount_data->bpb_ext.fat32.root_cluster; // For FAT32, the root directory starts at a specific cluster
  }

  mount_data->fat_start_sector = bpb->reserved_sector_count;
  mount_data->fat_size_sectors = mount_data->fat_type == FAT_TYPE_32 ? mount_data->bpb_ext.fat32.fat_size_32 : bpb->fat_size_sectors;
  mount_data->root_dir_start_sector = mount_data->fat_start_sector + (bpb->fat_count * mount_data->fat_size_sectors); // Root directory starts immediately after the FAT area
  mount_data->root_dir_sectors = ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector; // Calculate root directory size in sectors 
  mount_data->data_start_sector = bpb->reserved_sector_count + (bpb->fat_count * mount_data->fat_size_sectors) + mount_data->root_dir_sectors; // Calculate the starting sector of the data region
  mount_data->cluster_size_bytes = bpb->bytes_per_sector * bpb->sectors_per_cluster; // Calculate the size of a cluster in bytes
  mount_data->total_clusters = data_sectors / bpb->sectors_per_cluster; // Calculate the total number of clusters available

  block_release(buffer);
  return 0; // Success
}

int vfat_unmount(mount_t* mnt) {
  vfat_sync(mnt); // Ensure all data is flushed before unmounting
  kfree(mnt->private); // Free the BPB data allocated during mount

  return 0; // Success
}

int vfat_get_root(mount_t* mnt, vnode_t** vnode) {
  vnode_t* root;
  int res = vnode_get(mnt, 0, &root); // The root directory is typically represented by file_id 0 in FAT filesystems
  if (res) return res;

  root->v_type = VNODE_TYPE_DIRECTORY; // Set the vnode type to directory
  root->v_ops = &vfat_vnode_ops; // Assign the VFAT vnode operations to the root vnode

  vfat_node_data_t* root_info = kmalloc(sizeof(vfat_node_data_t));
  if (!root_info) {
    vnode_dec_ref(root); // Decrement ref count since we won't use it 
    return -ENOMEM;
  }

  vfat_mount_data_t* mount_info = (vfat_mount_data_t*)mnt->private;
  root_info->start_cluster = 0; // The root directory starts at cluster 0
  root_info->file_size = 0;
  root_info->attributes = 0x10; // Set the directory attribute
  root_info->is_root = true; // Mark this node as the root directory

  root->v_data = root_info; // Store the root directory info in the vnode's private data

  *vnode = root; // Return the root vnode to the caller
  return 0; // Success
}

int vfat_sync(mount_t* mnt) {
  return -ENOSYS; // Not implemented yet
}


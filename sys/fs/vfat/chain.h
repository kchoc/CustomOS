#ifndef FS_VFAT_CHAIN_H
#define FS_VFAT_CHAIN_H

#include "mount.h"

#include <fs/block.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/device.h>

// -----------------------------------------------------------------------
// Callback contract
// -----------------------------------------------------------------------
//
// Called once per sector window within the current cluster (or root-dir
// sector for FAT12/16).
//
//   dev        – opaque block-device handle passed to block_read/write
//   lba        – physical sector address to read or write
//   ctx        – caller-supplied opaque pointer
//
// Return 0 to continue the walk, VFAT_WALK_STOP to stop early, or a
// negative errno to abort with an error.
//
#define VFAT_WALK_STOP 1 /* positive sentinel — "done, not an error" */

typedef int (*vfat_walk_cb_t)(void* dev, uint64_t lba, void* ctx);

int vfat_walk_chain(device_t* dev, vfat_mount_data_t* mnt, uint32_t start_cluster,
                    vfat_walk_cb_t cb, void* ctx);

int vfat_follow_cluster(device_t* dev, vfat_mount_data_t* mnt, uint32_t cluster,
                        uint32_t* next_cluster);

bool vfat_is_eoc(vfat_mount_data_t* mnt, uint32_t cluster);

#endif // FS_VFAT_CHAIN_H

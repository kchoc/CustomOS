#include "chain.h"
#include "mount.h"

#include <sys/device.h>

#include <fs/block.h>
#include <kern/errno.h>
#include <stdint.h>
#include <string.h>

static int fat_read_sector(void* dev, uint64_t lba, void** blk)
{
    return block_read(dev, lba, blk, 512);
}

int vfat_follow_cluster(device_t* dev, vfat_mount_data_t* mnt, uint32_t cluster,
                        uint32_t* next_cluster)
{
    void* blk;

    switch (mnt->fat_type) {

    case FAT_TYPE_12: {
        uint32_t fat_offset   = cluster * 3 / 2;
        uint64_t fat_lba      = mnt->fat_start_sector + (fat_offset / 512);
        uint32_t entry_offset = fat_offset % 512;

        int res = fat_read_sector(dev, fat_lba, &blk);
        if (res)
            return res;

        // The 12-bit value may straddle two sectors.
        uint32_t val;
        if (entry_offset == 511) {
            // Straddles a sector boundary — need two reads.
            uint8_t lo = ((uint8_t*)blk)[511];
            block_release(blk);

            res = fat_read_sector(dev, fat_lba + 1, &blk);
            if (res)
                return res;

            uint8_t hi = ((uint8_t*)blk)[0];
            block_release(blk);
            val = lo | ((uint32_t)hi << 8);
        }
        else {
            val =
                ((uint8_t*)blk)[entry_offset] | ((uint32_t)((uint8_t*)blk)[entry_offset + 1] << 8);
            block_release(blk);
        }

        // Even clusters use the low 12 bits; odd clusters use the high 12.
        if (cluster & 1)
            val >>= 4;
        else
            val &= 0x0FFF;

        *next_cluster = val;
        return 0;
    }

    // ------------------------------------------------------------------
    case FAT_TYPE_16: {
        uint32_t fat_offset   = cluster * 2;
        uint32_t fat_lba      = mnt->fat_start_sector + (fat_offset / 512);
        uint32_t entry_offset = fat_offset % 512;

        int res = fat_read_sector(dev, fat_lba, &blk);
        if (res)
            return res;

        *next_cluster = *(uint16_t*)((uint8_t*)blk + entry_offset);
        block_release(blk);

        return 0;
    }

    case FAT_TYPE_32: {
        uint32_t fat_offset   = cluster * 4;
        uint32_t fat_lba      = mnt->fat_start_sector + (fat_offset / 512);
        uint32_t entry_offset = fat_offset % 512;

        int res = fat_read_sector(dev, fat_lba, &blk);
        if (res)
            return res;

        // FAT32 entries are 28 bits; mask the high 4 reserved bits.
        *next_cluster = (*(uint32_t*)((uint8_t*)blk + entry_offset)) & 0x0FFFFFFF;
        block_release(blk);
        return 0;
    }

    default:
        return -EINVAL;
    }
}

bool vfat_is_eoc(vfat_mount_data_t* mnt, uint32_t cluster)
{
    switch (mnt->fat_type) {
    case FAT_TYPE_12:
        return cluster >= 0xFF8;
    case FAT_TYPE_16:
        return cluster >= 0xFFF8;
    case FAT_TYPE_32:
        return cluster >= 0x0FFFFFF8;
    default:
        return true;
    }
}

static inline uint64_t cluster_to_lba(vfat_mount_data_t* mnt, uint32_t cluster)
{
    return ((uint64_t)(mnt->data_start_sector + (cluster - 2) * mnt->bpb.sectors_per_cluster));
}

int vfat_walk_chain(device_t* dev, vfat_mount_data_t* mnt, uint32_t start_cluster,
                    vfat_walk_cb_t cb, void* ctx)
{
    int status = 0;

    // ------------------------------------------------------------------
    // FAT12/16 fixed root directory — flat sector range, no cluster chain
    // ------------------------------------------------------------------
    if (start_cluster == 0 && mnt->fat_type != FAT_TYPE_32) {
        for (uint32_t s = (uint32_t)mnt->root_dir_start_sector;
             s < mnt->root_dir_sectors + mnt->root_dir_start_sector; s++) {

            status = cb(dev, s, ctx);
            if (status != 0)
                goto done;
        }
        goto done;
    }

    // ------------------------------------------------------------------
    // Normal clustered file / directory
    // ------------------------------------------------------------------
    uint32_t cluster = start_cluster;

    while (!vfat_is_eoc(mnt, cluster)) {
        uint64_t lba = cluster_to_lba(mnt, cluster);

        status = cb(dev, lba, ctx);
        if (status != 0)
            goto done;

        status = vfat_follow_cluster(dev, mnt, cluster, &cluster);
        if (status)
            goto done;
    }

done:
    return status == VFAT_WALK_STOP ? 0 : status;
}

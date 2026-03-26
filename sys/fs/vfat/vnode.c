#include "vnode.h"
#include "lookup.h"
#include "mount.h"

#include <fs/block.h>
#include <fs/mount.h>
#include <fs/vnode.h>

#include <vm/kmalloc.h>

#include <kern/errno.h>
#include <kern/terminal.h>

vnode_ops_t vfat_vnode_ops = {.lookup   = vfat_vnode_lookup,
                              .create   = vfat_vnode_create,
                              .link     = vfat_vnode_link,
                              .unlink   = vfat_vnode_unlink,
                              .rename   = DISALLOWED_OP,
                              .mkdir    = DISALLOWED_OP,
                              .rmdir    = DISALLOWED_OP,
                              .readdir  = DISALLOWED_OP,
                              .open     = DISALLOWED_OP,
                              .close    = DISALLOWED_OP,
                              .read     = vfat_vnode_read,
                              .write    = vfat_vnode_write,
                              .getattr  = DISALLOWED_OP,
                              .setattr  = DISALLOWED_OP,
                              .truncate = DISALLOWED_OP,
                              .access   = DISALLOWED_OP,
                              .symlink  = DISALLOWED_OP,
                              .readlink = DISALLOWED_OP,
                              .mknod    = DISALLOWED_OP,
                              .fsync    = DISALLOWED_OP,
                              .inactive = vfat_vnode_inactive,
                              .reclaim  = vfat_vnode_reclaim};

static int vfat_vnode_get(mount_t* mount, vfat_lookup_result_t* lookup_result, vnode_t** result)
{
    int res = vnode_get(mount, lookup_result->cluster, result);
    if (res)
        return res;

    if (!(*result)->v_data) {
        vfat_node_data_t* node_data = kmalloc(sizeof(vfat_node_data_t));
        if (!node_data) {
            vnode_dec_ref(*result);
            return -ENOMEM;
        }
        node_data->start_cluster  = lookup_result->cluster;
        node_data->file_size      = lookup_result->entry.file_size;
        node_data->attributes     = lookup_result->entry.attributes;
        node_data->is_root        = false; // This would need to be determined based on the entry
        node_data->cached_cluster = lookup_result->cluster;
        node_data->cached_index   = 0;
        (*result)->v_data         = node_data;
    }

    (*result)->v_ops = &vfat_vnode_ops;
    (*result)->v_type =
        (lookup_result->entry.attributes & 0x10) ? VNODE_TYPE_DIRECTORY : VNODE_TYPE_FILE;

    return 0;
}

int vfat_vnode_lookup(vnode_t* vp, const char* name, vnode_t** result)
{
    vfat_lookup_result_t lookup_result;

    vfat_node_data_t* node_data = (vfat_node_data_t*)vp->v_data;

    int res = vfat_lookup(vp, name, VFAT_LOOKUP_FIND, &lookup_result);
    if (res)
        return res;

    // Create a new vnode for the found entry
    res = vfat_vnode_get(vp->v_mount, &lookup_result, result);
    if (res)
        return res;

    return 0;
}

int vfat_vnode_link(vnode_t* dir, vnode_t* target, const char* name)
{
    return -EPERM; // Linking is not supported in VFAT
}

int vfat_vnode_unlink(vnode_t* dir, const char* name)
{
    return -EPERM; // Unlinking is not supported in VFAT
}

int vfat_vnode_read(vnode_t* node, void* buf, size_t size, size_t offset)
{
    vfat_node_data_t*  node_data  = (vfat_node_data_t*)node->v_data;
    vfat_mount_data_t* mount_data = (vfat_mount_data_t*)node->v_mount->private;
    uint32_t           current_cluster;
    size_t             file_size = node_data->file_size;

    if (offset >= file_size)
        return 0;
    if (offset + size > file_size)
        size = file_size - offset;
    if (size == 0)
        return 0;

    uint32_t clusters_to_skip      = offset / mount_data->cluster_size_bytes;
    size_t   offset_within_cluster = offset % mount_data->cluster_size_bytes;

    if (clusters_to_skip < node_data->cached_index) {
        // If the requested offset is before the cached cluster, we need to start from the beginning
        current_cluster = node_data->start_cluster;
    }
    else {
        // Start from the cached cluster
        current_cluster = node_data->cached_cluster;
        clusters_to_skip -= node_data->cached_index;
    }

    void* buffer;

    // Follow the cluster chain to find the starting cluster for reading
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        int res = follow_cluster_chain(node->v_mount->mnt_dev_vnode->v_data, mount_data,
                                       current_cluster, &current_cluster);
        if (res)
            return res;
        if (IS_CLUSTER_END(mount_data, current_cluster))
            return 0; // Reached end of file before reaching the desired offset
    }

    // Now read data from the current cluster, handling the offset within the cluster
    size_t bytes_read = 0;
    while (bytes_read < size) {
        uint64_t lba           = cluster_to_lba(mount_data, current_cluster);
        size_t   bytes_to_read = mount_data->cluster_size_bytes - offset_within_cluster;
        if (bytes_to_read > size - bytes_read)
            bytes_to_read = size - bytes_read;

        int res = block_read(node->v_mount->mnt_dev_vnode->v_data, lba, &buffer, 512);
        if (is_errno(res))
            return res;

        memcpy((char*)buf + bytes_read, (char*)buffer + offset_within_cluster, bytes_to_read);

        block_release(buffer);

        bytes_read += bytes_to_read;
        offset_within_cluster = 0; // Only the first cluster read may have a non-zero offset

        if (bytes_read < size) {
            res = follow_cluster_chain(node->v_mount->mnt_dev_vnode->v_data, mount_data,
                                       current_cluster, &current_cluster);
            if (res)
                return res;
            if (IS_CLUSTER_END(mount_data, current_cluster))
                break; // Reached end of file
        }
    }

    return bytes_read;
}

int vfat_vnode_write(vnode_t* node, const void* buf, size_t size, size_t offset)
{
    if (size == 0)
        return 0;
    if (node->v_type != VNODE_TYPE_FILE)
        return -EISDIR; // Cannot write to directories
    vfat_node_data_t*  node_data  = (vfat_node_data_t*)node->v_data;
    vfat_mount_data_t* mount_data = (vfat_mount_data_t*)node->v_mount->private;

    return -ENOSYS;
}

int vfat_vnode_create(vnode_t* dir, const char* name, vmode_t mode, vnode_t** result)
{
    vfat_mount_data_t* mount_data = (vfat_mount_data_t*)dir->v_mount->private;
    if (mount_data->flags & VFAT_FLAG_LFN_WRITE) {
        if (strlen(name) > 255)
            return -ENAMETOOLONG; // VFAT long filename limit
    }
    else {
        if (strlen(name) > 12)
            return -ENAMETOOLONG; // VFAT 8.3 filename limit
    }
    vfat_lookup_result_t lookup_result;
    int                  res = vfat_lookup(dir, name, VFAT_LOOKUP_FREE, &lookup_result);
    if (res)
        return res;

    void* buffer;
    if (mount_data->flags & VFAT_FLAG_LFN_WRITE) {

        return -ENOSYS;
    }
    else {
        res = block_read(dir->v_mount->mnt_dev_vnode->v_data, lookup_result.sector, &buffer, 512);
        if (res)
            return res;

        vfat_standard_entry_t* entry =
            (vfat_standard_entry_t*)((char*)buffer + lookup_result.offset);
        memset(entry, 0, sizeof(vfat_standard_entry_t));
        vfat_build_shortname((char*)entry->name, (const uint8_t*)name);
        entry->attributes         = 0;
        entry->first_cluster_low  = 0;
        entry->first_cluster_high = 0;
        entry->file_size          = 0;

        res = block_write(dir->v_mount->mnt_dev_vnode->v_data, lookup_result.sector, buffer, 512);
        block_release(buffer);
        if (res)
            return res;
    }

    return 0;
}

int vfat_vnode_inactive(vnode_t* node)
{
    // No special handling needed for now, but this is where we would clean up any resources
    // associated with the vnode if necessary
    return 0;
}

int vfat_vnode_reclaim(vnode_t* node)
{
    if (node->v_data) {
        kfree(node->v_data);
        node->v_data = NULL;
    }
    return 0;
}

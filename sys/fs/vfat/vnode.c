#include "vnode.h"
#include "chain.h"
#include "lookup.h"
#include "mount.h"

#include <fs/block.h>
#include <fs/file.h>
#include <fs/mount.h>
#include <fs/vnode.h>

#include <vm/kmalloc.h>
#include <kern/errno.h>
#include <kern/terminal.h>

#include <string.h>

/* =========================================================
   Forward declarations
   ========================================================= */
static ssize_t vfat_file_read (file_t* file, void* buf, size_t count);
static ssize_t vfat_file_write(file_t* file, const void* buf, size_t count);
static int     vfat_file_seek (file_t* file, loff_t offset, int whence);
static int     vfat_file_close(file_t* file);

/* =========================================================
   Ops tables
   ========================================================= */
vnode_ops_t vfat_vnode_ops = {
    .lookup   = vfat_vnode_lookup,
    .create   = vfat_vnode_create,
    .link     = vfat_vnode_link,
    .unlink   = vfat_vnode_unlink,
    .rename   = DISALLOWED_OP,
    .mkdir    = DISALLOWED_OP,
    .rmdir    = DISALLOWED_OP,
    .readdir  = vfat_vnode_readdir,
    .open     = vfat_vnode_open,
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
    .reclaim  = vfat_vnode_reclaim,
};

file_ops_t vfat_file_ops = {
    .read  = vfat_file_read,
    .write = vfat_file_write,
    .seek  = vfat_file_seek,
    .close = vfat_file_close,
    .ioctl = NULL,
};

void vfat_shortname_to_str(char* out, const uint8_t name[11])
{
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

/* =========================================================
   Read callback
   ========================================================= */
typedef struct {
    void*  dest;
    size_t dest_size;
    size_t copied;
    size_t file_size;
    size_t offset;
} read_ctx_t;

static int read_cb(device_t* dev, uint64_t lba, void* ctx)
{
    read_ctx_t* rc = (read_ctx_t*)ctx;

    uint32_t available = 512;
    if (rc->offset == 0)
      ;
    else if (rc->offset < 512) {
        available = 512 - rc->offset;
    } else {
        rc->offset -= 512;
        return 0;
    }

    if (rc->copied + available > rc->dest_size)
        available = rc->dest_size - rc->copied;
    if (available == 0)
        return VFAT_WALK_STOP;

    void* blk;
    int res = block_read(dev, lba, &blk, 512);
    if (res)
        return res;

    memcpy((char*)rc->dest + rc->copied, (char*)blk + rc->offset, available);
    block_release(blk);
    rc->offset = 0;

    rc->copied += available;
    return (rc->copied >= rc->dest_size) ? VFAT_WALK_STOP : 0;
}

/* =========================================================
   Readdir callback
   ========================================================= */
typedef struct {
    void*  buf;
    size_t buf_size;
    size_t out_off;
} readdir_ctx_t;

static int readdir_cb(void* dev, uint64_t lba, void* ctx)
{
    readdir_ctx_t* rc = (readdir_ctx_t*)ctx;

    void* blk;
    int res = block_read(dev, lba, &blk, 512);
    if (res)
        return res;

    size_t entry_count = (512) / sizeof(vfat_standard_entry_t);
    vfat_standard_entry_t* entries =
        (vfat_standard_entry_t*)((char*)blk);

    for (size_t i = 0; i < entry_count; i++) {
        vfat_standard_entry_t* e = &entries[i];

        if (e->name[0] == 0x00) {
            block_release(blk);
            return VFAT_WALK_STOP;   // end of directory
        }
        if ((uint8_t)e->name[0] == 0xE5) continue; // deleted
        if (e->attributes == VFAT_ATTR_LFN)  continue; // LFN entry
        if (e->attributes & VFAT_ATTR_VOLUME_ID) continue; // volume label

        char name[13];
        vfat_shortname_to_str(name, e->name);

        size_t name_len = strlen(name);
        if (rc->out_off + name_len + 1 > rc->buf_size) {
            block_release(blk);
            return VFAT_WALK_STOP;   // output buffer full
        }

        memcpy((char*)rc->buf + rc->out_off, name, name_len + 1);
        rc->out_off += name_len + 1;
    }

    block_release(blk);
    return 0;
}

/* =========================================================
   Lookup callback
   ========================================================= */
typedef struct {
    const char*           target;
    vfat_lookup_result_t  result;
    bool                  found;
} lookup_ctx_t;

static int lookup_cb(void* dev, uint64_t lba, void* ctx)
{
    lookup_ctx_t* lc = (lookup_ctx_t*)ctx;

    void* blk;
    int res = block_read(dev, lba, &blk, 512);
    if (res)
        return res;

    size_t entry_count = (512) / sizeof(vfat_standard_entry_t);
    vfat_standard_entry_t* entries =
        (vfat_standard_entry_t*)((char*)blk);

    for (size_t i = 0; i < entry_count; i++) {
        vfat_standard_entry_t* e = &entries[i];

        if (e->name[0] == 0x00) {
            block_release(blk);
            return VFAT_WALK_STOP;   // not found
        }
        if ((uint8_t)e->name[0] == 0xE5) continue;
        if (e->attributes == VFAT_ATTR_LFN) {
          printf("Warning: Found LFN entry while looking up '%s' - long filename support is not implemented, skipping\n",
                 lc->target);
          continue;
        }
        if (e->attributes & VFAT_ATTR_VOLUME_ID) continue;

        char name[13];
        vfat_shortname_to_str(name, e->name);

        if (strcmp(name, lc->target) == 0) {
            lc->result.entry   = *e;
            lc->result.cluster = ((uint32_t)e->first_cluster_high << 16) |
                                   e->first_cluster_low;
            lc->result.sector  = lba;
            lc->result.offset  = i * sizeof(vfat_standard_entry_t);
            lc->found = true;
            block_release(blk);
            return VFAT_WALK_STOP;
        }
    }

    block_release(blk);
    return 0;
}

/* =========================================================
   Internal vnode initialiser
   ========================================================= */
static int vfat_vnode_get(mount_t* mount, vfat_lookup_result_t* lr, vnode_t** out)
{
    int res = vnode_get(mount, lr->cluster, out);
    if (res)
        return res;

    if (!(*out)->v_data) {
        vfat_node_data_t* nd = kmalloc(sizeof(vfat_node_data_t));
        if (!nd) {
            vnode_dec_ref(*out);
            return -ENOMEM;
        }
        nd->start_cluster  = lr->cluster;
        nd->file_size      = lr->entry.file_size;
        nd->attributes     = lr->entry.attributes;
        nd->is_root        = false;
        nd->cached_cluster = lr->cluster;
        nd->cached_index   = 0;
        (*out)->v_data     = nd;
    }

    (*out)->v_ops  = &vfat_vnode_ops;
    (*out)->v_type = (lr->entry.attributes & VFAT_ATTR_DIR)
                         ? VNODE_TYPE_DIRECTORY
                         : VNODE_TYPE_FILE;
    return 0;
}

/* =========================================================
   vnode ops
   ========================================================= */

int vfat_vnode_open(vnode_t* vp, file_t* file)
{
    file->f_ops = &vfat_file_ops;
    return 0;
}

int vfat_vnode_lookup(vnode_t* vp, const char* name, vnode_t** result)
{
    char upper_name[13];
    if (strlen(name) > 12)
        return -ENAMETOOLONG;
    strncpy(upper_name, name, 13);
    strtoupper(upper_name);
    lookup_ctx_t lc = { .target = upper_name, .result = {0}, .found = false };

    int res = vfat_walk_chain(vp->v_mount->mnt_dev_vnode->v_data, vp->v_mount->private,
                              ((vfat_node_data_t*)vp->v_data)->start_cluster,
                              lookup_cb, &lc);

    if (res)
        return res;
    if (!lc.found)
        return -ENOENT;

    return vfat_vnode_get(vp->v_mount, &lc.result, result);
}

int vfat_vnode_read(vnode_t* vp, void* buf, size_t size, size_t offset)
{
    vfat_node_data_t* nd = (vfat_node_data_t*)vp->v_data;

    if (offset >= nd->file_size) return 0;
    if (offset + size > nd->file_size) size = nd->file_size - offset;
    if (size == 0) return 0;

    read_ctx_t rc = {
        .dest      = buf,
        .dest_size = size,
        .copied    = 0,
        .file_size = nd->file_size,
        .offset    = offset,
    };

    int res = vfat_walk_chain(vp->v_mount->mnt_dev_vnode->v_data, vp->v_mount->private,
                              nd->start_cluster, read_cb, &rc);
    if (res)
        return res;

    return (int)rc.copied;
}

int vfat_vnode_write(vnode_t* vp, const void* buf, size_t size, size_t offset)
{
    if (size == 0) return 0;
    if (vp->v_type != VNODE_TYPE_FILE) return -EISDIR;
    return -ENOSYS;
}

int vfat_vnode_readdir(vnode_t* vp, void* buf, size_t buf_size, size_t offset)
{
    if (vp->v_type != VNODE_TYPE_DIRECTORY)
        return -ENOTDIR;

    readdir_ctx_t rc = { .buf = buf, .buf_size = buf_size, .out_off = 0 };

    int res = vfat_walk_chain(vp->v_mount->mnt_dev_vnode->v_data, vp->v_mount->private,
                              ((vfat_node_data_t*)vp->v_data)->start_cluster,
                              readdir_cb, &rc);
    if (res)
        return res;

    return (int)rc.out_off;
}

int vfat_vnode_create(vnode_t* dir, const char* name, vmode_t mode, vnode_t** result)
{
    vfat_mount_data_t* mnt = (vfat_mount_data_t*)dir->v_mount->private;
    bool use_lfn = (mnt->flags & VFAT_FLAG_LFN_WRITE) != 0;

    if (use_lfn) {
        if (strlen(name) > 255) return -ENAMETOOLONG;
    } else {
        if (strlen(name) > 12) return -ENAMETOOLONG;
    }

    vfat_lookup_result_t lr;
    int res = vfat_lookup(dir, name, VFAT_LOOKUP_FREE, &lr);
    if (res)
        return res;

    if (use_lfn)
        return -ENOSYS;

    void* blk;
    res = block_read(dir->v_mount->mnt_dev_vnode->v_data, lr.sector, &blk, 512);
    if (res)
        return res;

    vfat_standard_entry_t* entry =
        (vfat_standard_entry_t*)((char*)blk + lr.offset);
    memset(entry, 0, sizeof(vfat_standard_entry_t));
    vfat_build_shortname((char*)entry->name, (const uint8_t*)name);
    entry->attributes         = 0;
    entry->first_cluster_low  = 0;
    entry->first_cluster_high = 0;
    entry->file_size          = 0;

    res = block_write(dir->v_mount->mnt_dev_vnode->v_data, lr.sector, blk, 512);
    block_release(blk);
    if (res)
        return res;

    if (result) {
        vfat_lookup_result_t new_lr;
        res = vfat_lookup(dir, name, VFAT_LOOKUP_FIND, &new_lr);
        if (res) return res;
        res = vfat_vnode_get(dir->v_mount, &new_lr, result);
        if (res) return res;
    }

    return 0;
}

int vfat_vnode_link   (vnode_t* dir, vnode_t* target, const char* name) { return -EPERM; }
int vfat_vnode_unlink (vnode_t* dir, const char* name)                   { return -EPERM; }
int vfat_vnode_inactive(vnode_t* node)                                   { return 0; }

int vfat_vnode_reclaim(vnode_t* node)
{
    if (node->v_data) {
        kfree(node->v_data);
        node->v_data = NULL;
    }
    return 0;
}

/* =========================================================
   file ops
   ========================================================= */

static ssize_t vfat_file_read(file_t* file, void* buf, size_t count)
{
    if (!file->f_vnode) return -EBADF;
    ssize_t bytes = vfat_vnode_read(file->f_vnode, buf, count, (size_t)file->f_pos);
    if (bytes > 0) file->f_pos += bytes;
    return bytes;
}

static ssize_t vfat_file_write(file_t* file, const void* buf, size_t count)
{
    if (!file->f_vnode) return -EBADF;
    ssize_t bytes = vfat_vnode_write(file->f_vnode, buf, count, (size_t)file->f_pos);
    if (bytes > 0) file->f_pos += bytes;
    return bytes;
}

static int vfat_file_seek(file_t* file, loff_t offset, int whence)
{
    if (!file->f_vnode) return -EBADF;

    vfat_node_data_t* nd = (vfat_node_data_t*)file->f_vnode->v_data;
    loff_t new_pos;

    switch (whence) {
    case SEEK_SET: new_pos = offset;                         break;
    case SEEK_CUR: new_pos = file->f_pos + offset;           break;
    case SEEK_END: new_pos = (loff_t)nd->file_size + offset; break;
    default:       return -EINVAL;
    }

    if (new_pos < 0) return -EINVAL;

    // Backwards seek — invalidate the cluster cache.
    if (new_pos < file->f_pos) {
        nd->cached_cluster = nd->start_cluster;
        nd->cached_index   = 0;
    }

    file->f_pos = new_pos;
    return 0;
}

static int vfat_file_close(file_t* file)
{
    return 0; // writeback goes here once write is implemented
}


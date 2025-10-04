#include "kernel/filesystem/vfs.h"
#include "kernel/filesystem/fat16.h"
#include "kernel/filesystem/file.h"

#include "kernel/memory/kmalloc.h"

#include "kernel/terminal.h"
#include "kernel/panic.h"
#include "kernel/types.h"
#include "types/list.h"
#include "types/string.h"

/* ========================
	VFS STRUCTURE ALLOCATORS
	======================== */

/**
 * Allocates and initializes a superblock structure.
 * @param fs_type Pointer to the filesystem type (can be NULL).
 * @param s_ops Pointer to the superblock operations (can be NULL).
 * @return Pointer to the allocated superblock, or NULL on failure.
 */
super_block_t* alloc_superblock(file_system_type_t* fs_type, const sb_ops_t* s_ops) {
	super_block_t* sb = kmalloc(sizeof(super_block_t));
	if (!sb) return NULL;

	sb->block_size = 4096; // Default block size
	sb->s_magic = 0; // To be set by filesystem
	sb->s_root = NULL; // To be set during mount
	sb->fs_type = fs_type;
	sb->s_op = s_ops;
	sb->private = NULL;

	return sb;
}

/** Allocates and initializes a dentry structure.
 * @param name Name of the directory entry (can be NULL).
 * @param inode Pointer to the associated inode (can be NULL).
 * @param parent Pointer to the parent dentry (can be NULL).
 * @return Pointer to the allocated dentry, or NULL on failure.
 */
dentry_t* alloc_dentry(const char* name, inode_t* inode, dentry_t* parent) {
	dentry_t* dentry = kmalloc(sizeof(dentry_t));
	if (!dentry) return NULL;

	dentry->d_parent =  parent;
	dentry->d_inode =   inode;
    dentry->d_name =    name ? strdup(name) : NULL;
	list_init(&dentry->children, false);
	dentry->node.prev = dentry->node.next = NULL;
	dentry->ref_count = 1;
	dentry->is_mountpoint = false;
	dentry->mnt_sb = NULL;
	dentry->d_sb = inode->i_sb;

	return dentry;
}

/** Allocates and initializes an inode structure.
 * @param ino Inode number.
 * @param mode File mode (permissions and type).
 * @param size Size of the file in bytes.
 * @param i_ops Pointer to the inode operations (can be NULL).
 * @param f_ops Pointer to the file operations (can be NULL).
 * @return Pointer to the allocated inode, or NULL on failure.
 */
inode_t* alloc_inode(unsigned long ino, umode_t mode, loff_t size,
						const inode_ops_t* i_ops, const file_ops_t* f_ops) {
	inode_t* inode = kmalloc(sizeof(inode_t));
	if (!inode) return NULL;

	inode->i_ino = ino;
	inode->i_mode = mode;
	inode->i_size = size;
	inode->i_ops = i_ops;
	inode->f_ops = f_ops;
	inode->ref_count = 1;
	inode->private = NULL;
	inode->i_sb = NULL;

	return inode;
}

/** Allocates and initializes a file structure.
 * @param dentry Pointer to the associated dentry (must not be NULL).
 * @param mode File mode (e.g., read, write).
 * @return Pointer to the allocated file, or NULL on failure.
 */
file_t* alloc_file(dentry_t* dentry, fmode_t mode) {
	file_t* file = kmalloc(sizeof(file_t));
	if (!file) return NULL;

	dentry->d_inode->ref_count++;
	dentry->ref_count++;

	file->f_dentry = dentry;
	file->f_inode = dentry->d_inode;
	file->f_ops = dentry->d_inode->f_ops;
	file->f_pos = 0;
	file->f_mode = mode;
	file->ref_count = 1;
	file->private = NULL;
	
	return file;
}

/* ====================
	VFS MOUNT MANAGEMENT
	==================== */

#define MAX_MOUNTS 128

static vfsmount_t *mount_table[MAX_MOUNTS];
static int mount_count = 0;

vfsmount_t *root_mnt = NULL;

/* -- Mount Table Management -- */

vfsmount_t* alloc_vfsmount(void) {
	if (mount_count >= MAX_MOUNTS) return NULL;
	vfsmount_t *mnt = kmalloc(sizeof(vfsmount_t));
	if (!mnt) return NULL;
	memset(mnt, 0, sizeof(vfsmount_t));
	mount_table[mount_count++] = mnt;
	return mnt;
}

int free_vfsmount(vfsmount_t *mnt) {
	if (!mnt) return -1;

	int idx = -1;
	for (int i = 0; i < mount_count; i++)
		if (mount_table[i] == mnt) { idx = i; break; }

	if (idx == -1) return -1; // Not found

	/* call put_super if provided */
    if (mnt->sb && mnt->sb->s_op && mnt->sb->s_op->put_super)
        mnt->sb->s_op->put_super(mnt->sb);

	kfree(mnt);
	mount_table[idx] = mount_table[--mount_count]; // Replace with last
	mount_table[mount_count] = NULL; // Not necessary but safer
	return 0;
}

vfsmount_t *lookup_mnt_for_dentry(dentry_t *dentry) {
	if (!dentry) return NULL;
	for (int i = 0; i < mount_count; i++) {
		if (mount_table[i] && mount_table[i]->mountpoint == dentry)
			return mount_table[i];
	}
	return NULL;
}

/** Mounts a filesystem at the specified mountpoint.
 * @param sb Pointer to the superblock of the filesystem to mount.
 * @param mountpoint Pointer to the dentry where the filesystem will be mounted (can be NULL for root).
 * @param mnt_root Pointer to the root dentry of the filesystem being mounted.
 * @return 0 on success, -1 on failure.
 */
int mount_fs(super_block_t* sb, dentry_t* mountpoint, dentry_t* mnt_root, device_t* device) {
	if (!sb || !mnt_root) return -1;

	vfsmount_t *mnt = alloc_vfsmount();
	if (!mnt) return -1;

	mnt->sb = sb;
	mnt->root = mnt_root;
	mnt->mountpoint = mountpoint;
	mnt->device = device;

	if (mountpoint) {
		mountpoint->is_mountpoint = true;
		mountpoint->mnt_sb = sb;
	}

	return 0;
}

/** Unmounts the filesystem mounted at the specified mountpoint.
 * @param mountpoint Pointer to the dentry where the filesystem is mounted.
 * @return 0 on success, -1 on failure.
 */
int unmount_fs(dentry_t* mountpoint) {
	if (!mountpoint || !mountpoint->is_mountpoint) return -1;

	vfsmount_t *mnt = lookup_mnt_for_dentry(mountpoint);
	if (!mnt) return -1;

	mountpoint->is_mountpoint = false;
	mountpoint->mnt_sb = NULL;
	return free_vfsmount(mnt);
}

/* --Drive Mount Management -- */

int vfs_mount_root(device_t* device) {
	if (root_mnt) return -1; // Already initialized

	file_system_type_t *fs_type = &fat16_fs_type;
	if (!fs_type) return -1;

	if (!fs_type->mount) return -1;

	dentry_t *root_dentry = fs_type->mount(fs_type, 0, "hd0", NULL);
	if (!root_dentry) return -1;

	root_mnt = alloc_vfsmount();
	if (!root_mnt) {
		dentry_cache_release(root_dentry);
		return -1;
	}

	root_mnt->sb = root_dentry->d_sb;
	root_mnt->root = root_dentry;
	root_mnt->mountpoint = NULL;

	return 0;
}

int vfs_mount_drive(device_t *device, const char *mount_path, const char *fs_name) {
	file_system_type_t *fs_type = get_fs_type(fs_name);
	if (!fs_type || !fs_type->mount) return -1;

	if (mount_path == NULL || mount_path[0] == '\0') {
		if (root_mnt) return -1; // Root already mounted
		return vfs_mount_root(device);
	}

	dentry_t *mnt_point = vfs_lookup(NULL, mount_path);
	if (!mnt_point) return -1;

	dentry_t *mnt_root = fs_type->mount(fs_type, 0, device->name, NULL);
	if (!mnt_root || !mnt_root->d_inode) return -1;

	if (mount_fs(mnt_root->d_sb, mnt_point, mnt_root, device)) {
		dentry_cache_release(mnt_root);
		return -1;
	}

	return 0;
}

/* --------------------
	DENTRY CACHE
  	-------------------- */

#define DCACHE_LOOKUP(dentry, parent, name) \
	dentry_t* dentry __cleanup(pdentry_cache_release) = dentry_cache_lookup(parent, name);


/** Looks up a dentry in the cache under the specified parent by name.
 * @param parent Pointer to the parent dentry.
 * @param name Name of the dentry to look up.
 * @return Pointer to the found dentry with incremented ref_count, or NULL if not found.
 */
dentry_t *dentry_cache_lookup(dentry_t *parent, const char *name) {
	if (!parent || !name) return NULL;

	list_node_t *node;
	list_for_each(node, &parent->children) {
		dentry_t *child = (dentry_t *)(node - offsetof(dentry_t, node));
		if (child && child->d_name && strcmp(child->d_name, name) == 0) {
			child->ref_count++;
			return child;
		}
	}
	return NULL;
}

/** Releases a dentry from the cache, decrementing its reference count and freeing it if no longer used.
 * @param dentry Pointer to the dentry to release.
 */
void dentry_cache_release(dentry_t *dentry) {
	if (!dentry) return;

	dentry->ref_count--;
	if (dentry->ref_count > 0) return;

	// Remove from parent's children list
	if (dentry->d_parent) list_remove(&dentry->node);

	// Free associated inode
	list_node_t *node;
	list_for_each(node, &dentry->children) {
		dentry_t *child = (dentry_t *)(node - offsetof(dentry_t, node));
		dentry_cache_release(child);
	}

	if (dentry->d_inode) {
		dentry->d_inode->ref_count--;
		if (dentry->d_inode->ref_count <= 0)
			kfree(dentry->d_inode);
	}


	if (dentry->d_name) kfree(dentry->d_name);
	kfree(dentry);
}

void pdentry_cache_release(dentry_t **dentry) {
	if (dentry && *dentry) {
		dentry_cache_release(*dentry);
		*dentry = NULL;
	}
}

/* --------------------
	VFS PATH RESOLUTION
	-------------------- */

dentry_t *vfs_lookup(dentry_t *start, const char *path) {
	if (!path || path[0] == '\0') return NULL;

	// Make a modifiable copy of path and free it automatically on going out of scope
	char* path_copy __cleanup(kfreep) = strdup(path);
	if (!path_copy) return NULL;

	dentry_t *current = start ? start : root_mnt->root;
	dentry_t *next = NULL;
	char *dir = strtok((char *)path_copy, "/");

	if (!dir) return current; // Path is "/"

    do {
    	if (strcmp(dir, ".") == 0)
    		continue;

    	if (strcmp(dir, "..") == 0) {
    		if (current->d_parent) current = current->d_parent;
			continue;
		}

		// Check cache first
		next = dentry_cache_lookup(current, dir);

		// If not in cache, use lookup operation
		if (!next) {
			next = current->d_inode->i_ops->lookup(current->d_inode, dir, 0);
			if (next) {
				next->d_parent = current;
				list_push_head(&current->children, &next->node);
			}
		}

		if (!next) return NULL; // Not found

		if (next->is_mountpoint && next->mnt_sb) {
			vfsmount_t *mnt = lookup_mnt_for_dentry(next);
			if (mnt && mnt->root)
				current = mnt->root;
			continue;
		}

		current = next;

    } while ((dir = strtok(NULL, "/")));

    return current;
}

/* --------------------
	VFS ENTRY OPERATIONS
  	-------------------- */

int vfs_mkdir(inode_t *dir, dentry_t *dentry, umode_t mode) {
	if (!dir || !dentry || !dentry->d_name || dentry->d_name[0] == '\0') return -1;
	if (!dir->i_ops || !dir->i_ops->mkdir) return -1;

	return dir->i_ops->mkdir(dir, dentry, mode);	
}

int vfs_rmdir(inode_t *dir, dentry_t *dentry) {
	if (!dir || !dentry || !dentry->d_name || dentry->d_name[0] == '\0') return -1;
	if (!dir->i_ops || !dir->i_ops->rmdir) return -1;

	return dir->i_ops->rmdir(dir, dentry);
}

int vfs_create(inode_t *dir, dentry_t* dentry, umode_t mode, bool excl) {
	if (!dir || !dentry || !dentry->d_name || dentry->d_name[0] == '\0') return -1;
	if (!dir->i_ops || !dir->i_ops->create) return -1;

	return dir->i_ops->create(dir, dentry, mode, excl);
}

int vfs_unlink(inode_t *dir, dentry_t *dentry) {
	if (!dir || !dentry || !dentry->d_name || dentry->d_name[0] == '\0') return -1;
	if (!dir->i_ops || !dir->i_ops->unlink) return -1;

	return dir->i_ops->unlink(dir, dentry);
}

int vfs_rename(inode_t *old_dir, dentry_t *old_dentry, inode_t *new_dir, dentry_t *new_dentry, unsigned int flags) {
	if (!old_dir || !old_dentry || !old_dentry->d_name || old_dentry->d_name[0] == '\0') return -1;
	if (!new_dir || !new_dentry || !new_dentry->d_name || new_dentry->d_name[0] == '\0') return -1;
	if (!old_dir->i_ops || !old_dir->i_ops->rename) return -1;

	return old_dir->i_ops->rename(old_dir, old_dentry, new_dir, new_dentry, flags);
}

/* -------------------
   VFS FILE OPERATIONS
   ------------------- */

file_t *vfs_open(const char *path, int flags, umode_t mode) {
	if (!path || path[0] == '\0') return NULL;

	dentry_t *dentry = vfs_lookup(NULL, path);
	if (!dentry) return NULL;
	if (!dentry->d_inode) return NULL;

	file_t *file = alloc_file(dentry, flags);
	if (!file) return NULL;

	if (file->f_ops && file->f_ops->open) {
		if (file->f_ops->open(file->f_inode, file) != 0) {
			kfree(file);
			return NULL;
		}
	}

	return file;
}

void vfs_close(file_t *file) {
	if (!file) return;

	if (file->f_ops && file->f_ops->release)
		file->f_ops->release(file->f_inode, file);

	file->ref_count--;
	if (file->ref_count <= 0) {
		dentry_cache_release(file->f_dentry);
		kfree(file);
	}
}

ssize_t vfs_read(file_t *file, void __user *buf, size_t count, loff_t *offset) {
	if (!file || !buf) return -1;
	if (!file->f_ops || !file->f_ops->read) return -1;

	return file->f_ops->read(file, buf, count, offset);
}

ssize_t vfs_write(file_t *file, const void __user *buf, size_t count, loff_t *offset) {
	if (!file || !buf) return -1;
	if (!file->f_ops || !file->f_ops->write) return -1;

	return file->f_ops->write(file, buf, count, offset);
}

int vfs_llseek(file_t *file, loff_t offset, int whence) {
	if (!file) return -1;
	if (!file->f_ops || !file->f_ops->llseek) return -1;

	int res = file->f_ops->llseek(file, offset, whence);
	if (res == 0) {
		file->f_pos = offset;
	}
	return res;
}

/* --------------------
	VFS MISC
	-------------------- */

void vfs_print_mounts(void) {
	printf("Mounted filesystems:\n");
	for (int i = 0; i < mount_count; i++) {
		vfsmount_t *mnt = mount_table[i];
		if (!mnt) continue;
		if (mnt->mountpoint && mnt->mountpoint->d_name)
			printf(" %d: on %s (fs type: %s)\n", i, mnt->mountpoint->d_name, mnt->sb->fs_type->name);
		else
			printf(" %d: root (fs type: %s)\n", i, mnt->sb->fs_type->name);
	}
}

void vfs_print_tree(dentry_t *dir, int depth) {
	if (!dir) return;

	for (int i = 0; i < depth; i++) printf("  ");
	printf("|- %s\n", dir->d_name ? dir->d_name : "(null)");

	list_node_t *node;
	list_for_each(node, &dir->children) {
		dentry_t *child = (dentry_t *)node;
		vfs_print_tree(child, depth + 1);
	}
}

bool vfs_print_directory_entry(dir_context_t *ctx, const char *name, int namelen, uint32_t ino, uint32_t file_size, unsigned type) {
	// Print the name
	printf(" %s (size: %uB, inode: %u, type: %s)\n", name, file_size, ino,
	       (type == 1) ? "DIR" : (type == 2) ? "SYMLINK" : "FILE");
	ctx->pos++;
	return true;
}

void vfs_ls(const char *path) {
	if (!path || path[0] == '\0') path = "/";

	file_t *file = vfs_open(path, 0, 0x4000); // Read-only, directory
	if (!file) {
		printf("vfs_ls: Failed to open path: %s\n", path);
		return;
	}

	if ((file->f_inode->i_mode & 0x4000) == 0) { // Not a directory
		printf("vfs_ls: Not a directory: %s\n", path);
		vfs_close(file);
		return;
	}

	dir_context_t ctx = { .pos = 0, .actor = vfs_print_directory_entry };
	if (file->f_ops && file->f_ops->iterate_shared) {
		file->f_ops->iterate_shared(file, &ctx);
	} else {
		printf("vfs_ls: No readdir operation for this filesystem\n");
	}
	vfs_close(file);
}

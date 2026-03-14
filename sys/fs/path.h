#ifndef FS_PATH_H
#define FS_PATH_H

typedef struct dentry dentry_t;

typedef struct path {
	dentry_t *dentry;
} path_t;

int format_filename_83(const char* name, char out[11]);

#endif // FS_PATH_H

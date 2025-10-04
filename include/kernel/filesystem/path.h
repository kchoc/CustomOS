#ifndef PATH_H
#define PATH_H

#include <stdint.h>

typedef struct dentry dentry_t;

typedef struct path {
	dentry_t *dentry;
} path_t;

int format_filename_83(const char* name, char out[11]);

#endif // PATH_H

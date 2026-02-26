#ifndef FILE_H
#define FILE_H

#include <inttypes.h>

typedef uint32_t loff_t;
typedef uint32_t fmode_t;
typedef uint16_t umode_t;

typedef struct dentry dentry_t;
typedef struct inode inode_t;
typedef struct file_operations file_ops_t;

typedef struct file {
	fmode_t 			f_mode;
	const file_ops_t 	*f_ops;
	dentry_t 			*f_dentry;
	inode_t 			*f_inode;
	loff_t 				f_pos;
	int 				ref_count;
	void 				*private;
} file_t;

#endif // FILE_H
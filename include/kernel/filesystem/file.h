#ifndef FILE_H
#define FILE_H

#include "kernel/types.h"

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
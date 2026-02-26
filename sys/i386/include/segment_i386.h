#ifndef SEGMENT_I386_H
#define SEGMENT_I386_H

#include <x86/include/segment.h>

typedef struct region_descriptor {
	unsigned rd_limit : 16;
	unsigned rd_base  : 32;
} __packed region_desc_t;

#endif // SEGMENT_I386_H

#ifndef _I386_SEGMENT_H_
#define _I386_SEGMENT_H_

#include <machine/segment.h>

typedef struct region_descriptor {
	unsigned rd_limit : 16;
	unsigned rd_base  : 32;
} __packed region_desc_t;

#endif // _I386_SEGMENT_H_

#ifndef x86_I386_GDT_H
#define x86_I386_GDT_H

#include "machine/segment_i386.h"
#include <inttypes.h>
#include <machine/segment.h>

extern seg_desc_t gdt[NGDT];

void lgdt(region_desc_t* gdt_desc);

#endif // x86_I386_GDT_H

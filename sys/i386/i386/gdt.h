#ifndef I386_GDT_H
#define I386_GDT_H

#include <machine/segment_i386.h>
#include <machine/segment.h>

#include <inttypes.h>

extern seg_desc_t gdt[NGDT];

void lgdt(region_desc_t* gdt_desc);

#endif // I386_GDT_H

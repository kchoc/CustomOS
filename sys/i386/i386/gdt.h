#ifndef x86_I386_GDT_H
#define x86_I386_GDT_H

#include <inttypes.h>
#include <machine/segment.h>

extern seg_desc_t gdt[NGDT];

void lgdt(seg_desc_t* gdt_desc);

#endif // x86_I386_GDT_H

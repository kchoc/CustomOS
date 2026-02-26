#include "tss.h"
#include "gdt.h"

#include <string.h>
#include <inttypes.h>

void write_tss(seg_desc_t *gdt, int num, tss_t *tss,
               uint32_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)tss;
    uint32_t limit = sizeof(tss_t) - 1;

    SET_SEGMENT_BASE(gdt[num], base);
    SET_SEGMENT_LIMIT(gdt[num], limit);
    gdt[num].sd_granularity   = 0x0;  // Byte granularity
    gdt[num].sd_dpl          = 0x0;  // Kernel level
    gdt[num].sd_present      = 0x1;  // Segment present
    gdt[num].sd_size         = 0x1;  // 32-bit segment
    gdt[num].sd_type         = 0x9;  // Available 32-bit TSS

    memset(tss, 0, sizeof(tss_t));
    tss->ss0 = ss0;
    tss->esp0 = esp0;
    tss->cs = 0x08; // Kernel mode code segment
    tss->ss = tss->ds = tss->es = tss->fs = tss->gs = 0x10; // Kernel mode data segment
    tss->iomap_base = sizeof(tss_t);
}

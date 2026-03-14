#include "tss.h"

#include <string.h>

void write_tss(tss_t *tss, uint32_t ss0, uint32_t esp0) {
    memset(tss, 0, sizeof(tss_t));
    tss->ss0 = ss0;
    tss->esp0 = esp0;
    tss->cs = 0x08; // Kernel mode code segment
    tss->ss = tss->ds = tss->es = tss->fs = tss->gs = 0x10; // Kernel mode data segment
    tss->iomap_base = sizeof(tss_t);
}

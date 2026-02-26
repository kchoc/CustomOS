#ifndef X86_I386_GENASSYM_H
#define X86_I386_GENASSYM_H

#include <sys/assym.h>

// The build system is being weird so we have to define these constants without the macros
// GDT Offsets
ASSYM(KCSEL, 0x20); // GSEL(GCODE_SEL, SEL_KPL)
ASSYM(KDSEL, 0x28); // GSEL(GDATA_SEL, SEL_KPL)
ASSYM(KPSEL, 0x8);  // GSEL(GPRIV_SEL, SEL_KPL)

#endif // X86_I386_GENASSYM_H

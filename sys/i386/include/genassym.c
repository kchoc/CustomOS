#include <sys/pcpu.h>
#include <machine/segment.h>

#ifdef __GENASSYM__
#include <sys/assym.h>

int main() {
  ASSYM(UCSEL, GSEL(GUCODE_SEL, SEL_UPL));
  ASSYM(UDSEL, GSEL(GUDATA_SEL, SEL_UPL));
  ASSYM(KCSEL, GSEL(GCODE_SEL, SEL_KPL));
  ASSYM(KDSEL, GSEL(GDATA_SEL, SEL_KPL));
  ASSYM(KPSEL, GSEL(GDATA_SEL, SEL_KPL));

  ASSYM(SCHEDULER_LOCK_OFFSET, offsetof(pcpu_t, scheduler_lock));
  ASSYM(PCPU_ESP0_OFFSET, offsetof(pcpu_t, tss.esp0));
  return 0;
}
#endif


#include "kernel/descriptors/bda.h"
#include "kernel/panic.h"
#include "types/string.h"

bda_t* bda = (bda_t*)0x400;
ebda_t* ebda = NULL;

void load_ebda() {
	if (bda->ebda_segment)
		ebda = (ebda_t*)((uintptr_t)(bda->ebda_segment) << 4);
	else
		PANIC("No EBDA segment found in BDA!");

}

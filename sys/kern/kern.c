#include "kern.h"

void mi_init() {
	while (1) {
		asm volatile("nop");
	}
}

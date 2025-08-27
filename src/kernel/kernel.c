#include "kernel/arch.h"

void spindle() {
	while (1);
}

void main() {
	init();

    spindle();
}

#include "kernel/arch.h"
#include "kernel/task.h"

void spindle() {
	while (1);
}

void main() {
	init();

    spindle();
}

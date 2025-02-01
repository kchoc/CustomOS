#include "kernel/arch.h"
#include "kernel/task.h"
#include "kernel/terminal.h"

void task1() {
	for (int i = 0; i < 3; i++) {
        	printf("Task 1\n");
	}
}

void task2() {
	for (int i = 0; i < 2; i++) {
        	printf("Task 2\n");
	}
}

void main() {
	init();

	create_task(&task1);
	create_task(&task2);

	while (1) {
        	// Do nothing
	}
}

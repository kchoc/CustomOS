#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef struct task {
	uint32_t id;			// Task ID
	uint32_t state; 		// 0 = running, 1 = ready, 2 = blocked
	uint32_t priority;		// Task priority
	uint32_t esp, ebp, eip;	// Stack and instruction pointers
	uint32_t cr3;			// Page directory
	struct task *next;		// Next task in the list
} task_t;

void init_tasking();
int switch_task();
int create_task();

#endif // TASK_H

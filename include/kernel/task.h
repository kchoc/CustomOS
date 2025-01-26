#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef struct task {
	uint32_t id;
	uint32_t esp, ebp;
	uint32_t eip;
	struct task *next;
} task_t;

void init_tasking();
int switch_task();
void create_task(void (*entry)());
void terminate_task();
void list_tasks();

#endif // TASK_H

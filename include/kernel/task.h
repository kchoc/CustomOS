#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "kernel/memory/page.h"

#define MAX_TASKS 10

typedef struct thread_control_block {
    uint32_t *esp;      // Offset used: [TCB.ESP]        -> saved kernel stack pointer
    page_table_t  *cr3;  // Offset used: [TCB.CR3]        -> page directory (for virtual memory)
    uint32_t  esp0;      // Offset used: [TCB.ESP0]       -> top of kernel stack (for privilege changes)
    // ... additional fields as needed
} tcb_t;

typedef struct task {
	uint32_t id;			// Task ID
	tcb_t* tcb;
	struct task *next;		// Next task in the list
} task_t;

extern task_t *current_task;
extern void switch_to(tcb_t* next);

void spindle();
void tasking_init();
task_t* create_task(void (*entry)(void));
void create_task_from_binary(const char *binary_path);
void switch_task();
int  exit_task(task_t* task);

#endif // TASK_H

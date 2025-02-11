#include "kernel/task.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
#include "kernel/terminal.h"

task_t *current_task;
task_t *ready_queue;
uint32_t next_task_id = 1;

void init_tasking() {
    // Allocate space for the first task
    current_task = (task_t *)kmalloc(sizeof(task_t));
    current_task->id = next_task_id++;
    current_task->esp = 0;
    current_task->ebp = 0;
    current_task->eip = 0;
	current_task->cr3 = (uint32_t)current_page_directory;
    current_task->next = current_task;
    ready_queue = current_task;
}

int switch_task() {
	// If we haven't initialized tasking yet, return
	if (!current_task) return 0;

	task_t *next_task = current_task->next;
	if (next_task == current_task) return 0;

	// Save the current task's state
	asm volatile(
        "mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        : "=r"(current_task->esp),
		"=r"(current_task->ebp)
	);

	// Switch to the next task
	current_task = next_task;

	// Restore the next task's state
	asm volatile(
		"mov %0, %%cr3\n"
		"mov %1, %%ebp\n"
        "mov %2, %%esp\n"
        :: "r"(current_task->cr3),
		"r"(current_task->ebp),
		"r"(current_task->esp)
	);

	return 1;
}

int create_task() {
    // Create a new page directory
    page_directory *pd = current_page_directory;
    if (!pd) return 0;

    for (int i = 0; i < 256; i++) {
        //pd->entries[i + 758] = kernel_tables[i].entries[i];
    }

    // Allocate space for the new task
    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->esp = 0;
    new_task->ebp = 0;
    new_task->eip = 0;
    new_task->cr3 = (uint32_t)pd;
    
    // Add the new task to the ready queue
    task_t *temp = ready_queue;
    while (temp->next != ready_queue) {
        temp = temp->next;
    }
    temp->next = new_task;

    return new_task->id;
}

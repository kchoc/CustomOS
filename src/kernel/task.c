#include "kernel/task.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
#include "kernel/memory/physical_memory.h"
#include "kernel/terminal.h"

task_t *current_task;
task_t *ready_queue;

void init_tasking() {
    // Allocate space for the first task
    current_task = (task_t *)kmalloc(sizeof(task_t));
    current_task->id = 0;
    current_task->esp = 0;
    current_task->ebp = 0;
    current_task->eip = 0;
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
        	: "=r"(current_task->esp), "=r"(current_task->ebp)
	);

	// Switch to the next task
	current_task = next_task;

	// Restore the next task's state
	asm volatile(
        	"mov %0, %%esp\n"
        	"mov %1, %%ebp\n"
        	"jmp *%2\n"
        	:: "r"(current_task->esp), "r"(current_task->ebp), "r"(current_task->eip)
	);

	return 1;
}

void create_task(void (*entry)()) {
	// Create page directory and page table for the new task
	page_directory *new_pd = (page_directory *)allocate_blocks(3);
	page_table *new_pt = (page_table *)allocate_blocks(1);


	// Allocate space for the new task
	task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
	new_task->id = current_task->id + 1;
	new_task->esp = (uint32_t)kmalloc(4096) + 4096;
	new_task->ebp = new_task->esp;
	new_task->eip = (uint32_t)entry;
	new_task->next = ready_queue->next;

	ready_queue->next = new_task;
}

void terminate_task() {
	// Remove the task from the ready queue
	task_t *prev_task = current_task;
	while (prev_task->next != current_task) {
		prev_task = prev_task->next;
	}
	prev_task->next = current_task->next;

	// Free the task's stack and task structure
	kfree((void *)current_task->esp - 4096);
	kfree(current_task);

	// Switch to the next task
	current_task = prev_task->next;
	switch_task();
}

void list_tasks() {
	task_t *task = ready_queue;
	do {
		printf("Task %d\n", task->id);
		task = task->next;
	} while (task != ready_queue);
}

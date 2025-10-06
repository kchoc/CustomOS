#ifndef TASK_H
#define TASK_H

#include "types/list.h"
#include "kernel/types.h"

#define MAX_TASKS 100

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_SLEEPING,
    TASK_STOPPED,
    TASK_ZOMBIE
} task_state_t;

typedef struct fxsave_state {
    uint8_t fx_region[512] __attribute__((aligned(16)));
} fxss_t;

typedef struct process_control_block {
    uint32_t esp;   // Stack pointer
    uint32_t ebp;   // Base pointer
    uint32_t eip;   // Instruction pointer
    uint32_t cr3;   // Page directory base register
    uint32_t esp0;  // Kernel stack top

    fxss_t fxstate;
} pcb_t;

struct process;

typedef struct thread {
    list_node_t node; // For linking threads in a list
    list_node_t proc_node; // For linking in process's thread list
    pcb_t tcb;
    struct process* proc;

    uint32_t tid;       // Thread ID
    uint8_t state;      // Thread state (e.g., running, ready, blocked)
    uint8_t priority;   // Thread priority

    void *kstack;       // Kernel stack pointer
    uint32_t kstack_size; // Kernel stack size
} thread_t;

typedef struct vm_space vm_space_t;
typedef struct list list_t;

typedef struct process {
    list_node_t node; // For linking processes in a list
    uint32_t pid;       // Process ID
    uint32_t ppid;      // Parent process ID
    char name[32];      // Process name

    vm_space_t *vmspace;    // Memory management info
    // ... other process-specific information

    thread_t* main_thread;  // Main thread of the process
    list_t threads;      // Linked list of threads in the process
} proc_t;

typedef struct cpu cpu_t;

extern proc_t* idle_process;

extern void switch_to(pcb_t *prev, pcb_t *next);

void idle_task(void);
void tasking_init();
thread_t* create_task(void (*entry)(void), proc_t *p, uint32_t priority, cpu_t* cpu);
proc_t* create_process(const char* name);
cpu_t* select_cpu();
void yeild();
void schedule();
void thread_exit();
void list_tasks();
void list_cpu_threads(cpu_t* cpu);

void free_thread(cpu_t* cpu, thread_t *t);
void free_process(proc_t *proc);

#endif // TASK_H

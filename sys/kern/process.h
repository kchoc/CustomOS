#ifndef TASK_H
#define TASK_H

#include <machine/context.h>
#include <machine/trapframe.h>

#include <libkern/common.h>

#include <list.h>
#include <inttypes.h>

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,    // Waiting for I/O (stdin, etc.)
    TASK_SLEEPING,
    TASK_STOPPED,
    TASK_ZOMBIE
} task_state_t;

struct process;

typedef struct thread {
    list_node_t node; // For linking threads in a list
    list_node_t proc_node; // For linking in process's thread list
    struct process* proc;

    context_t* context; // CPU context for this threads
    trapframe_t* trapframe; // Pointer to saved registers (used during scheduling and stored on the kernel stack)

    uint32_t tid;       // Thread ID
    uint8_t state;      // Thread state (e.g., running, ready, blocked)
    uint8_t priority;   // Thread priority

    void *kstack;       // Kernel stack pointer
    uint32_t kstack_size; // Kernel stack size
} thread_t;

typedef struct vm_space vm_space_t;
typedef struct list list_t;
typedef struct fd_table fd_table_t;

typedef struct process {
    list_node_t node; // For linking processes in a list
    uint32_t pid;       // Process ID
    uint32_t ppid;      // Parent process ID
    char name[32];      // Process name

    vm_space_t *vmspace;    // Memory management info
    fd_table_t *fd_table;   // File descriptor table

    list_t threads;      // Linked list of threads in the process
} proc_t;

typedef struct wait_node {
    list_node_t node;
    thread_t* thread;
} wait_node_t;

typedef struct pcpu pcpu_t;

extern proc_t* idle_process;

void idle_task(void);
void tasking_init();
thread_t* create_kernel_thread(void (*entry)(void), proc_t *p, uint32_t priority, pcpu_t* pcpu);
thread_t* create_user_thread(void (*entry)(void), proc_t *p, uint32_t priority, pcpu_t* pcpu);
proc_t* create_process(const char* name);
pcpu_t* select_pcpu();
void schedule_from_irq(registers_t *regs);
void thread_exit(registers_t* regs);
void list_tasks();
void list_pcpu_threads(pcpu_t* pcpu);

void free_thread(thread_t *t);
void free_process(proc_t *proc);

/* Wait queue for blocking operations */
void block_current_thread(list_t* wait_queue);
void wake_up_queue(list_t* wait_queue);

#endif // TASK_H

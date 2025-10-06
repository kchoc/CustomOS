#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vmspace.h"
#include "kernel/process/process.h"
#include "kernel/process/elf.h"
#include "kernel/process/cpu.h"
#include "kernel/terminal.h"
#include "kernel/panic.h"
#include "types/string.h"
#include "types/list.h"

#define STACK_SIZE 4096
#define MAX_PROGRAM_SIZE (1024 * 1024) // 1 MB
#define PROGRAM_ENTRY (void (*)(void))0x00001000

static list_t* all_processes = NULL;

proc_t* idle_process = NULL;

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

static inline thread_t* proc_node_to_thread(list_node_t* node) {
    if (!node) return NULL;
    return (thread_t*)((uint8_t*)node - offsetof(thread_t, proc_node));
}

void idle_task() {
    while (1) {
        yeild();
    }
}

/* ---------------- Runqueue Management ---------------- */

// Add a thread to the runqueue (circular linked list); handle empty list case
static void runqueue_add(cpu_t* cpu, thread_t *t) {
    list_push_tail(&cpu->runqueue, (list_node_t*)&t->node);
    cpu->total_priority += t->priority;
}

static void runqueue_remove(cpu_t* cpu, thread_t *t) {
    list_node_t* node = list_find(&cpu->runqueue, &t->node);
    if (!node) return;
    list_remove(node);
    cpu->total_priority -= t->priority;
}

/* ---------------- Process Management ---------------- */

// Add a process to the global process list
static void proc_list_add(proc_t* p) {
    list_push_tail(all_processes, (list_node_t*)&p->node);
}

// Remove a process from the global process list
static void proc_list_remove(proc_t* p) {
    list_node_t* node = list_find(all_processes, &p->node);
    if (!node) return;
    list_remove(node);
}

// Add a thread to its process thread list
static void proc_add_thread(proc_t* p, thread_t* t) {
    list_push_tail(&p->threads, (list_node_t*)&t->proc_node);
}

// Remove a thread from its process thread list
static void proc_remove_thread(proc_t* p, thread_t* t) {
    list_node_t* node = list_find(&p->threads, &t->proc_node);
    if (!node) return;
    list_remove(node);
}

/* ---------------- Allocation / Creation ---------------- */
void tasking_init() {
    all_processes = kmalloc(sizeof(list_t));
    memset(all_processes, 0, sizeof(list_t));

    // bootstrap "idle" thread representing the kernel at startup
    proc_t *p = create_process("idle");
    idle_process = p;

    vm_space_t* old = vm_space_switch(p->vmspace);

    thread_t *t = create_task(idle_task, p, 0, &cpus[0]);

    cpus[0].current_thread = (thread_t*)&t->node;

    
    p->main_thread = t;
    proc_add_thread(p, t);

    vm_space_switch(old);
}

thread_t* create_task(void (*entry)(void), proc_t *p, uint32_t priority, cpu_t* cpu) {
    thread_t* t = kmalloc(sizeof(thread_t));
    memset(t, 0, sizeof(*t));
    t->tid = next_tid++;
    t->state = TASK_READY;
    t->priority = priority;
    t->proc = p;

    uint8_t* stack = kmalloc(STACK_SIZE);
    memset(stack, 0, STACK_SIZE);
    t->kstack = stack;
    t->kstack_size = STACK_SIZE;

    uint32_t *stk = (uint32_t *)(stack + STACK_SIZE);
    
    *(--stk) = (uint32_t)thread_exit; // Return address
    *(--stk) = (uint32_t)entry; // EIP
    *(--stk) = 0; // EBX
    *(--stk) = 0; // ESI
    *(--stk) = 0; // EDI
    *(--stk) = 0; // EBP

    t->tcb.esp = (uint32_t)stk;
    t->tcb.esp0 = (uint32_t)(stack + STACK_SIZE);
    t->tcb.cr3 = (uint32_t)p->vmspace->page_directory;

    proc_add_thread(p, t);
    if (!cpu) cpu = select_cpu();
    runqueue_add(cpu, t);

    return t;
}

proc_t* create_process(const char* name) {
    proc_t *p = kmalloc(sizeof(proc_t));
    memset(p, 0, sizeof(*p));
    p->pid = next_pid++;
    strncpy(p->name, name, 11);

    p->vmspace = vm_space_create();

    proc_list_add(p);
    return p;
}

cpu_t* select_cpu() {
    cpu_t* lowest = &cpus[0];
    for (uint32_t i = 1; i < cpu_count; i++) {
        if (cpus[i].total_priority < lowest->total_priority)
            lowest = &cpus[i];
    }
    return lowest;
}

/* ---------------- Freeing / Reaping ---------------- */

// Free a single thread (assumes it's already removed from runqueue and proc list)
void free_thread(cpu_t* cpu, thread_t *t) {
    if (!t) return;

    if (t == cpu->current_thread)
        PANIC("Attempted to free the current running thread!");

    if (t->kstack) kfree(t->kstack);    
    kfree(t);
}

// Free a process and all its threads (assumes threads are removed from runqueue)
void free_process(proc_t *p) {
    if (!p) return;

    proc_list_remove(p);

    // Remove all threads
    thread_t* node;
    do {
        node = proc_node_to_thread(list_pop_head(&p->threads));
    }  while (node && (node->state = TASK_ZOMBIE));

    if (p->vmspace)
        vm_space_destroy(p->vmspace);

    kfree(p);
}

// Reap all zombie threads in the runqueue
static void reap_zombies(cpu_t* cpu) {
    thread_t* t = (thread_t*)cpu->runqueue.head;
    if (!t) return;

    do {
        if (t != cpu->current_thread && t->state == TASK_ZOMBIE) {
            list_remove((list_node_t*)&t->node);
            if (t->proc) {
                proc_remove_thread(t->proc, t);
                if (t->proc->threads.head == NULL)
                    free_process(t->proc);
            }
            free_thread(cpu, t);
        }
        t = (thread_t*)t->node.next;
    } while (t && t != cpu->current_thread);
}

/* ---------------- Scheduling ---------------- */

void thread_exit() {
    get_current_cpu()->current_thread->state = TASK_ZOMBIE;
    yeild();

    PANIC("Thread exit failed to yield");
}

void schedule() {
    // printf("Scheduling... %u\n", get_current_cpu()->apic_id);
    //TODO: Reaping each time is inefficient; consider doing it less frequently
    cpu_t* cpu = get_current_cpu();
    reap_zombies(cpu);

    thread_t *prev = cpu->current_thread;
    if (!prev)
        PANIC("No current thread to schedule from!");

    thread_t *next = (thread_t*)prev->node.next;
    if (!next)
        next = (thread_t*)cpu->runqueue.head;

    if (next == prev) {
        if (prev->state == TASK_ZOMBIE)
            PANIC("No runnable threads available!");

        return; // Only one thread, continue running it
    }

    thread_t *start = next;
    while (next->state != TASK_READY && next != prev) {
        next = (thread_t*)next->node.next;
        if (next == start) break;
    }

    if (next == prev || next->state != TASK_READY) return;

    if (prev->state == TASK_RUNNING) prev->state = TASK_READY;

    cpu->current_thread = next;
    next->state = TASK_RUNNING;

    switch_to(&prev->tcb, &next->tcb);
}

void yeild() {
    schedule();
}

/* ---------------- Debugging / Listing ---------------- */

void list_tasks() {
    printf("CPU    PID   TID   PPID  STATE     NAME\n");
    printf("=========================================\n");

    list_node_t* node = all_processes->head;
    while (node) {
        proc_t* p = (proc_t*)node;
        thread_t* t = proc_node_to_thread(p->threads.head);
        while (t) {
            const char* state_str = "UNKNOWN";
            switch (t->state) {
                case TASK_RUNNING: state_str = "RUNNING"; break;
                case TASK_READY:   state_str = "READY";   break;
                case TASK_STOPPED: state_str = "BLOCKED"; break;
                case TASK_SLEEPING: state_str = "SLEEPING"; break;
                case TASK_ZOMBIE:  state_str = "ZOMBIE";  break;
            }
            printf("UNKNOWN %u     %u     %u     %s     %s\n", p->pid, t->tid, p->ppid, state_str, p->name);
            t = proc_node_to_thread(t->proc_node.next);
        }
        node = node->next;
    }
}

void list_cpu_threads(cpu_t* cpu) {
    printf("CPU %u Runqueue:\n", cpu->apic_id);
    printf("TID   PID   PPID  STATE     NAME\n");
    printf("================================\n");

    list_node_t* node = cpu->runqueue.head;
    if (!node) {
        printf("<empty>\n");
        return;
    }

    do {
        thread_t* t = (thread_t*)node;
        proc_t* p = t->proc;
        const char* state_str = "UNKNOWN";
        switch (t->state) {
            case TASK_RUNNING: state_str = "RUNNING"; break;
            case TASK_READY:   state_str = "READY";   break;
            case TASK_STOPPED: state_str = "BLOCKED"; break;
            case TASK_SLEEPING: state_str = "SLEEPING"; break;
            case TASK_ZOMBIE:  state_str = "ZOMBIE";  break;
        }
        printf("%u     %u     %u     %s     %s\n", t->tid, p->pid, p->ppid, state_str, p->name);
        node = node->next;
    } while (node && node != cpu->runqueue.head);
}
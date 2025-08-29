#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vmspace.h"
#include "kernel/process/process.h"
#include "kernel/process/elf.h"
#include "kernel/terminal.h"
#include "kernel/panic.h"
#include "types/string.h"
#include "types/list.h"
#include <stdint.h>

#define STACK_SIZE 4096
#define MAX_PROGRAM_SIZE (1024 * 1024) // 1 MB
#define PROGRAM_ENTRY (void (*)(void))0x00001000

thread_t *current_thread = NULL;
static thread_t *runqueue_head = NULL;
static thread_t *runqueue_tail = NULL;
static list_t* all_processes = NULL;

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

void idle_task() {
    while (1) {
        yeild();
    }
}

/* ---------------- Runqueue Management ---------------- */

// Add a thread to the runqueue (circular linked list); handle empty list case
static void runqueue_add(thread_t *t) {
    if (!runqueue_head) {
        runqueue_head = runqueue_tail = t;
        t->next = t; // circular list
    } else {
        runqueue_tail->next = t;
        t->next = runqueue_head;
        runqueue_tail = t;
    }
}

// Remove a thread from the runqueue; handle single element case
static uint32_t runqueue_remove(thread_t *t) {
    if (!runqueue_head) return -1; // empty

    thread_t *prev = runqueue_tail;
    thread_t *curr = runqueue_head;

    do {
        if (curr == t) {
            if (curr == runqueue_head && curr == runqueue_tail) {
                runqueue_head = runqueue_tail = NULL; // list becomes empty
            } else {
                prev->next = curr->next;
                if (curr == runqueue_head) runqueue_head = curr->next;
                if (curr == runqueue_tail) runqueue_tail = prev;
            }
            curr->next = NULL;
            return 0; // success
        }
        prev = curr;
        curr = curr->next;
    } while (curr && curr != runqueue_head);

    return -1; // not found
}

static void remove_thread(thread_t* prev) {
    if (!prev || !prev->next) return;

    thread_t* curr = prev->next;
    if (prev == curr) {
        // Only one element in the list
        runqueue_head = runqueue_tail = NULL;
    } else {
        prev->next = curr->next;
        if (curr == runqueue_head) runqueue_head = curr->next;
        if (curr == runqueue_tail) runqueue_tail = prev;
    }
}

/* ---------------- Process Management ---------------- */

// Add a process to the global process list
static void proc_list_add(proc_t* p) {
    list_node_t* node = kmalloc(sizeof(list_node_t));
    node->data = p;
    list_push_tail(all_processes, node);
}

// Remove a process from the global process list
static void proc_list_remove(proc_t* p) {
    list_node_t* current = all_processes->head;
    while (current) {
        if (current->data == p) {
            list_remove(current);
            kfree(current);
            return;
        }
        current = current->next;
    }
}

// Count threads in a process
static int proc_thread_count(proc_t* p) {
    int count = 0;
    thread_t* t = p->threads;
    while (t) {
        count++;
        t = t->next_in_proc;
    }
    return count;
}

// Remove a thread from its process thread list
static void proc_remove_thread(proc_t* p, thread_t* t) {
    thread_t **link = &p->threads;
    while (*link) {
        if (*link == t) {
            *link = t->next_in_proc;
            t->next_in_proc = NULL;
            return;
        }
        link = &(*link)->next_in_proc;
    }
}

/* ---------------- Allocation / Creation ---------------- */
void tasking_init() {
    all_processes = kmalloc(sizeof(list_t));
    memset(all_processes, 0, sizeof(list_t));

    // bootstrap "idle" thread representing the kernel at startup
    proc_t *p = create_process("idle");

    vm_space_t* old = vm_space_switch(p->vmspace);

    thread_t *t = create_task(idle_task, p);

    p->main_thread = t;
    p->threads = t;

    // set globals
    current_thread = t;
    t->next_in_proc = NULL;

    vm_space_switch(old);
}

thread_t* create_task(void (*entry)(void), proc_t *p) {
    thread_t* t = kmalloc(sizeof(thread_t));
    memset(t, 0, sizeof(*t));
    t->tid = next_tid++;
    t->state = TASK_READY;
    t->priority = 0;
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

    t->next_in_proc = p->threads;
    p->threads = t;

    runqueue_add(t);

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

/* ---------------- Freeing / Reaping ---------------- */

// Free a single thread (assumes it's already removed from runqueue and proc list)
void free_thread(thread_t *t) {
    if (!t) return;

    if (t == current_thread) {
        PANIC("Attempted to free the current running thread!");
    }

    if (t->kstack) kfree(t->kstack);
    kfree(t);
}

// Free a process and all its threads (assumes threads are removed from runqueue)
void free_process(proc_t *p) {
    if (!p) return;

    proc_list_remove(p);

    // Remove all threads
    thread_t *t = p->threads;
    while (t) {
        thread_t *next = t->next_in_proc;
        if (t != current_thread) {
            if (t->kstack) kfree(t->kstack);
            kfree(t);
        } else {
            /* current thread still exists; should not happen when freeing process */
            PANIC("Attempt to free process containing current_thread");
        }
        t = next;
    }

    if (p->vmspace)
        vm_space_destroy(p->vmspace);

    kfree(p);
}

// Reap all zombie threads in the runqueue
static void reap_zombies() {
    if (!runqueue_head) return;

    thread_t *prev = runqueue_tail;
    thread_t *curr = runqueue_head;
    thread_t* next;

    do {
        next = curr->next;

        if (curr != current_thread && curr->state == TASK_ZOMBIE) {
            remove_thread(prev);
            if (curr->proc) {
                proc_remove_thread(curr->proc, curr);
                if (proc_thread_count(curr->proc) == 0)
                    free_process(curr->proc);
            }
            free_thread(curr);
        } else {
            prev = curr;
        }

        curr = next;
    } while (curr && curr != runqueue_head);
}

/* ---------------- Scheduling ---------------- */

void thread_exit() {
    current_thread->state = TASK_ZOMBIE;
    yeild();

    PANIC("Thread exit failed to yield");
}

void schedule() {
    //TODO: Reaping each time is inefficient; consider doing it less frequently
    reap_zombies();

    thread_t *prev = current_thread;
    if (!prev)
        PANIC("No current thread to schedule from!");

    thread_t *next = prev->next;
    if (!next)
        PANIC("Runqueue is empty!");

    if (next == prev) {
        if (prev->state == TASK_ZOMBIE)
            PANIC("No runnable threads available!");

        return; // Only one thread, continue running it
    }

    thread_t *start = next;
    while (next->state != TASK_READY && next != prev) {
        next = next->next;
        if (next == start) break;
    }

    if (next == prev || next->state != TASK_READY) return;

    if (prev->state == TASK_RUNNING) prev->state = TASK_READY;

    current_thread = next;
    current_thread->state = TASK_RUNNING;

    switch_to(&prev->tcb, &next->tcb);
}

void yeild() {
    schedule();
}

/* ---------------- Debugging / Listing ---------------- */

void list_tasks() {
    printf("PID   TID   PPID  STATE     NAME\n");
    printf("=====================================\n");

    list_node_t* node = all_processes->head;
    while (node) {
        proc_t* p = (proc_t*)node->data;
        thread_t* t = p->threads;
        while (t) {
            const char* state_str = "UNKNOWN";
            switch (t->state) {
                case TASK_RUNNING: state_str = "RUNNING"; break;
                case TASK_READY:   state_str = "READY";   break;
                case TASK_STOPPED: state_str = "BLOCKED"; break;
                case TASK_SLEEPING: state_str = "SLEEPING"; break;
                case TASK_ZOMBIE:  state_str = "ZOMBIE";  break;
            }
            printf("%u     %u     %u     %s     %s\n", p->pid, t->tid, p->ppid, state_str, p->name);
            t = t->next_in_proc;
        }
        node = node->next;
    }

}

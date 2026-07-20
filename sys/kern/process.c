#include "process.h"
#include "elf.h"
#include "fd.h"
#include "panic.h"
#include "terminal.h"

#include <sys/pcpu.h>

#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <machine/gdt.h>

#include <kern/errno.h>

#include <list.h>
#include <string.h>

#define STACK_SIZE        4096
#define USER_STACK_TOP    0xC0000000 // 3GB
#define USER_STACK_BOTTOM (USER_STACK_TOP - STACK_SIZE)

vaddr_t user_stack_bottom = USER_STACK_BOTTOM;

static list_t all_processes = LIST_INIT_START(&idle_process.node);
proc_t        idle_process  = {
    .pid      = 0,
    .ppid     = 0,
    .name     = "idle",
    .threads  = LIST_INIT,
    .vmspace  = &kernel_vm_space,
    .fd_table = NULL,
    .node     = LIST_NODE_INIT(&all_processes),
};

thread_t idle_thread = {
    .tid         = 0,
    .state       = TASK_RUNNING,
    .priority    = 0,
    .kstack      = NULL,
    .kstack_size = 0,
    .context     = NULL,
    .trapframe   = NULL,
    .node        = {0},
    .proc_node   = LIST_NODE_INIT(&idle_process.threads),
};

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

thread_t* create_kernel_thread(void (*entry)(void), proc_t* p, uint32_t priority, pcpu_t* pcpu)
{
    if (!p)
        return NULL;

    thread_t* t = kmalloc(sizeof(thread_t));
    if (!t)
        return NULL;
    memset(t, 0, sizeof(*t));

    t->tid      = next_tid++;
    t->state    = TASK_READY;
    t->priority = priority;

    t->kstack = kmalloc(STACK_SIZE);
    if (!t->kstack) {
        kfree(t);
        return NULL;
    }
    memset(t->kstack, 0, STACK_SIZE);
    t->kstack_size = STACK_SIZE;

    uint32_t* stk = (uint32_t*)(t->kstack + STACK_SIZE);
    context_init(t, entry, stk, 0);

    list_push_tail(&p->threads, &t->proc_node);

    if (!pcpu)
        pcpu = select_pcpu();
    list_push_tail(&pcpu->runqueue, &t->node);

    return t;
}

thread_t* create_user_thread(void (*entry)(void), proc_t* p, uint32_t priority, pcpu_t* pcpu,
                             void* user_stack_top)
{
    if (!p)
        return NULL;

    thread_t* t = kmalloc(sizeof(thread_t));
    if (!t)
        return NULL;
    memset(t, 0, sizeof(*t));

    t->tid      = next_tid++;
    t->state    = TASK_READY;
    t->priority = priority;

    t->kstack = kmalloc(STACK_SIZE);
    if (!t->kstack) {
        kfree(t);
        return NULL;
    }
    memset(t->kstack, 0, STACK_SIZE);
    t->kstack_size = STACK_SIZE;

    uint32_t* stk = (uint32_t*)(t->kstack + STACK_SIZE);
    context_init(t, entry, stk, (uint32_t)user_stack_top); // Start user stack at the top

    list_push_tail(&p->threads, &t->proc_node);
    if (!pcpu)
        pcpu = select_pcpu();
    list_push_tail(&pcpu->runqueue, &t->node);
    pcpu->total_priority += priority;

    return t;
}

thread_t* fork_user_thread(thread_t* parent_thread, proc_t* child_proc, uint32_t priority,
                           pcpu_t* pcpu)
{
    if (!parent_thread || !child_proc)
        return NULL;

    thread_t* t = kmalloc(sizeof(thread_t));
    if (!t)
        return NULL;
    memset(t, 0, sizeof(*t));

    t->tid      = next_tid++;
    t->state    = TASK_READY;
    t->priority = priority;

    t->kstack = kmalloc(STACK_SIZE);
    if (!t->kstack) {
        kfree(t);
        return NULL;
    }
    memset(t->kstack, 0, STACK_SIZE);
    t->kstack_size = STACK_SIZE;

    uint32_t* stk = (uint32_t*)(t->kstack + STACK_SIZE);

    context_fork(
        t, parent_thread,
        stk); // Set up context to start at start_fork with a copy of the parent's trapframe

    list_push_tail(&child_proc->threads, &t->proc_node);
    if (!pcpu)
        pcpu = select_pcpu();
    list_push_tail(&pcpu->runqueue, &t->node);
    pcpu->total_priority += priority;

    return t;
}

proc_t* create_process(const char* name)
{
    proc_t* p = kmalloc(sizeof(proc_t));
    if (!p)
        return NULL;
    memset(p, 0, sizeof(*p));
    p->pid  = next_pid++;
    p->ppid = 0; // For now, no parent-child relationships
    if (name) {
        // Copy name with safety
        strncpy(p->name, name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
    }

    list_init(&p->threads, 0);
    p->vmspace = vm_space_fork(&kernel_vm_space); // Start with a copy of the kernel VM space

    // Create file descriptor table
    p->fd_table = fd_table_create();
    if (!p->fd_table) {
        vm_space_destroy(p->vmspace);
        kfree(p);
        return NULL;
    }

    // Initialize standard file descriptors
    fd_init_stdio(p);

    list_push_tail(&all_processes, &p->node);
    return p;
}

int fork_process(thread_t* t, int flags, proc_t** child_out)
{
    if (!t)
        return -EINVAL;

    proc_t* parent = get_proc_from_thread(t);

    proc_t* child = kmalloc(sizeof(proc_t));
    if (!child)
        return -ENOMEM;
    memset(child, 0, sizeof(*child));

    child->pid  = next_pid++;
    child->ppid = parent->pid;

    strncpy(child->name, parent->name, sizeof(child->name) - 1);
    child->name[sizeof(child->name) - 1] = '\0';

    child->vmspace = vm_space_fork(parent->vmspace);
    if (IS_ERR(child->vmspace)) {
        kfree(child);
        return (int)child->vmspace;
    }
    child->fd_table = fd_table_fork(parent->fd_table);
    if (IS_ERR(child->fd_table)) {
        vm_space_destroy(child->vmspace);
        kfree(child);
        return (int)child->fd_table;
    }

    printf("Child process cr3: 0x%08x\n", child->vmspace->arch->pd);

    list_init(&child->threads, 0);
    list_push_tail(&all_processes, &child->node);

    // Create a new thread for the child process that starts at the same entry point as the parent
    thread_t* child_thread = fork_user_thread(t, child, t->priority, NULL);
    if (!child_thread) {
        list_remove(&child->node);
        vm_space_destroy(child->vmspace);
        fd_table_destroy(child->fd_table);
        kfree(child);
        return -ENOMEM;
    }

    if (child_out)
        *child_out = child;

    return 0;
}

pcpu_t* select_pcpu()
{
    pcpu_t* lowest = &pcpus[0];
    return lowest;
    for (uint32_t i = 1; i < cpu_count; i++) {
        if (pcpus[i].total_priority < lowest->total_priority)
            lowest = &pcpus[i];
    }
    return lowest;
}

/* ---------------- Freeing / Reaping ---------------- */

// Free a single thread (assumes it's already removed from runqueue and proc list)
void free_thread(thread_t* t)
{
    if (!t)
        return;
    pcpu_t* pcpu = get_pcpu_from_thread(t);
    if (pcpu && t == pcpu->current_thread)
        PANIC("Attempted to free the current running thread!");

    proc_t* p = get_proc_from_thread(t);

    list_remove(&t->proc_node); // Remove from process thread list
    list_remove(&t->node);      // Remove from runqueue if still present
    pcpu->total_priority -= t->priority;

    if (p && p->threads.size == 0)
        free_process(p);

    if (t->kstack)
        kfree(t->kstack);
    kfree(t);
}

void free_process(proc_t* p)
{
    if (!p)
        return;

    printf("Freeing process %d (%s)\n", p->pid, p->name);

    list_remove(&p->node); // Remove from global process list

    if (p->vmspace)
        vm_space_destroy(p->vmspace);

    if (p->fd_table)
        fd_table_destroy(p->fd_table);

    kfree(p);
}

/* ---------------- Scheduling ---------------- */

thread_t* get_next_ready_thread(thread_t* prev)
{
    thread_t* next = thread_from_runqueue_node(prev->node.next);
    if (!next)
        PANIC("get_next_ready_thread: Current thread not in runqueue or runqueue is empty");

    thread_t* start = next;
    while (next->state != TASK_READY) {
        if (next->state == TASK_ZOMBIE && next != PCPU_GET(current_thread)) {
            thread_t* to_free = next;
            next              = thread_from_runqueue_node(next->node.next);
            free_thread(to_free);
        }
        else {
            next = thread_from_runqueue_node(next->node.next);
        }
        if (!next || next == start)
            return NULL;
    }
    return next;
}

void thread_exit(registers_t* regs)
{
    PCPU_GET(current_thread)->state = TASK_ZOMBIE;
    schedule_from_irq(regs);
    PANIC("thread_exit: Returned from scheduler after marking thread as zombie, this should never "
          "happen!");
}

void yield()
{
    asm volatile("int $0x20" : : "a"(0)); // Trigger scheduler interrupt (vector 64)
}

void schedule_from_irq(registers_t* regs)
{
    pcpu_t* pcpu = get_pcpu();

    if (spin_trylock(&pcpu->scheduler_lock) != 0)
        return;

    thread_t* prev = pcpu->current_thread;
    thread_t* next = get_next_ready_thread(prev);

    if (!next || next == prev) {
        spin_unlock(&pcpu->scheduler_lock);
        return;
    }

    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;

    proc_t* next_proc = get_proc_from_thread(next);

    // Switch address space first so memory accesses to next's memory are correct
    vm_space_activate(next_proc->vmspace);

    // Update TSS.ESP0 so interrupts land on next kernel stack
    pcpu->tss.esp0 = (uint32_t)(next->kstack + next->kstack_size);

    // Set the CPU's notion of current thread
    pcpu->current_thread = next;

    // Load general purpose regs for next thread
    next->state = TASK_RUNNING;

    // terminal_display_scheduler_info(next);

    context_switch(&prev->context, next->context);
    spin_unlock(&pcpu->scheduler_lock);
}

/* ---------------- Wait Queue (for blocking I/O) ---------------- */

// // Block the current thread and add it to a wait queue
// void block_current_thread(list_t* wait_queue)
// {
//     thread_t* current = PCPU_GET(current_thread);
//     if (!current)
//         return;
//
//     // Change state to blocked
//     current->state = TASK_BLOCKED;
//
//     // Add to wait queue if provided
//     if (wait_queue) {
//         wait_node_t* wait_node = kmalloc(sizeof(wait_node_t));
//         if (!wait_node)
//             PANIC("block_current_thread: Out of memory");
//         wait_node->thread = current;
//         list_push_head(wait_queue, &wait_node->node);
//     }
//
//     // Trigger a software interrupt to invoke the scheduler from interrupt context
//     // This ensures proper context saving/restoration via schedule_from_irq()
//     // Timer interrupt is on vector 64 (0x40)
//     __asm__ __volatile__("int $0x40");
// }
//
// // Wake up all threads in a wait queue
// void wake_up_queue(list_t* wait_queue)
// {
//     if (!wait_queue || !wait_queue->head)
//         return;
//
//     list_node_t* node = wait_queue->head;
//     while (node) {
//         wait_node_t* wait_node = WAIT_NODE_FROM_NODE(node);
//         thread_t*    t         = wait_node->thread;
//         if (t && t->state == TASK_BLOCKED) {
//             t->state = TASK_READY;
//         }
//
//         list_node_t* to_free = node;
//         node                 = node->next;
//         list_remove(to_free);
//         kfree(wait_node);
//     }
// }

/* ---------------- Debugging / Listing ---------------- */

void list_tasks()
{
    printf("Process count: %u\n", next_pid - 1);
    printf("CPU   PID   TID   PPID  STATE    NAME\n");
    printf("==========================================\n");

    list_node_t* node = all_processes.head;
    while (node) {
        proc_t* p = get_proc_from_node(node);
        printf("      %5u\n", p->pid);
        thread_t* t = thread_from_proc_node(p->threads.head);
        while (t) {
            const char* state_str = "UNKNOWN";
            switch (t->state) {
            case TASK_RUNNING:
                state_str = "RUNNING ";
                break;
            case TASK_READY:
                state_str = "READY   ";
                break;
            case TASK_STOPPED:
                state_str = "BLOCKED ";
                break;
            case TASK_SLEEPING:
                state_str = "SLEEPING";
                break;
            case TASK_ZOMBIE:
                state_str = "ZOMBIE  ";
                break;
            }
            proc_t* p = get_proc_from_thread(t);
            printf("%5u %5u %5u %5u %s %s\n", get_pcpu_from_thread(t)->pc_cpu_id, p->pid, t->tid,
                   p->ppid, state_str, p->name);

            t = thread_from_proc_node(t->proc_node.next);
        }
        node = node->next;
    }
}

void list_pcpu_threads(pcpu_t* pcpu)
{
    printf("CPU %u Runqueue:\n", pcpu->pc_cpu_id);
    printf("TID   PID   PPID  STATE    NAME\n");
    printf("====================================\n");

    list_node_t* node = pcpu->runqueue.head;
    if (!node) {
        printf("<empty>\n");
        return;
    }

    do {
        thread_t*   t         = thread_from_runqueue_node(node);
        proc_t*     p         = get_proc_from_thread(t);
        const char* state_str = "UNKNOWN";
        switch (t->state) {
        case TASK_RUNNING:
            state_str = " RUNNING";
            break;
        case TASK_READY:
            state_str = "   READY";
            break;
        case TASK_BLOCKED:
            state_str = " BLOCKED";
            break;
        case TASK_STOPPED:
            state_str = " STOPPED";
            break;
        case TASK_SLEEPING:
            state_str = "SLEEPING";
            break;
        case TASK_ZOMBIE:
            state_str = "  ZOMBIE";
            break;
        }
        printf("%5u %5u %5u %s %s\n", t->tid, p->pid, p->ppid, state_str, p->name);

        node = node->next;
    } while (node && node != pcpu->runqueue.head);
}

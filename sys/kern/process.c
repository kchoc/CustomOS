#include "process.h"
#include "elf.h"
#include "fd.h"
#include "panic.h"
#include "terminal.h"

#include <sys/pcpu.h>

#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <machine/gdt.h>

#include <string.h>
#include <list.h>

#define STACK_SIZE 4096
#define USER_STACK_TOP 0xC0000000 // 3GB
#define USER_STACK_BOTTOM (USER_STACK_TOP - STACK_SIZE)

vaddr_t user_stack_bottom = USER_STACK_BOTTOM;


static list_t all_processes = {0};
proc_t* idle_process = NULL;

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

// Helpers to convert between list nodes and thread/process structures
#define PCPU_FROM_TASK(t) ((pcpu_t*)(t->node.list - offsetof(pcpu_t, runqueue)))
#define GET_RUNQUEUE (list_t*)(&PCPU_GET(runqueue) - offsetof(pcpu_t, runqueue))
#define PROC_FROM_NODE(n) ((n) ? (proc_t*)((uint8_t*)n - offsetof(proc_t, node)) : NULL)
#define THREAD_FROM_NODE(n) ((n) ? (thread_t*)((uint8_t*)n - offsetof(thread_t, node)) : NULL)
#define THREAD_FROM_PROC_NODE(n) ((n) ? (thread_t*)((uint8_t*)n - offsetof(thread_t, proc_node)) : NULL)
#define WAIT_NODE_FROM_NODE(n) ((n) ? (wait_node_t*)((uint8_t*)n - offsetof(wait_node_t, node)) : NULL)

void idle_task() {
    while (1) {
      asm volatile("hlt");
    }
}

/* ---------------- Allocation / Creation ---------------- */
void tasking_init(void) {
    list_init(&all_processes, 0);

    // bootstrap "idle" thread and process
    proc_t *p = create_process("idle");
    if (!p) PANIC("Failed to create idle process!");
    idle_process = p;

    thread_t* t = kmalloc(sizeof(thread_t));
    if (!t) PANIC("Failed to create idle thread!");
    memset(t, 0, sizeof(*t));
    
    t->tid = next_tid++;
    t->state = TASK_RUNNING;
    t->priority = 0;
    t->proc = p;

    t->kstack = 0; // Idle thread uses the existing kernel stack set up by the boot code
    t->kstack_size = 0; // No need to free this stack since it's statically allocated and shared

    list_push_tail(&p->threads, &t->proc_node);
    list_push_tail(&PCPU_GET(runqueue), &t->node);
    PCPU_SET(current_thread, t);
}

thread_t* create_kernel_thread(void (*entry)(void), proc_t *p, uint32_t priority, pcpu_t* pcpu) {
    if (!p) return NULL;

    thread_t* t = kmalloc(sizeof(thread_t));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));

    t->tid = next_tid++;
    t->state = TASK_READY;
    t->priority = priority;
    t->proc = p;

    t->kstack = kmalloc(STACK_SIZE);
    if (!t->kstack) { kfree(t); return NULL; }
    memset(t->kstack, 0, STACK_SIZE);
    t->kstack_size = STACK_SIZE;

    uint32_t *stk = (uint32_t *)(t->kstack + STACK_SIZE);
    context_init(t, entry, stk, 0); 

    list_push_tail(&p->threads, &t->proc_node);
    if (!pcpu) pcpu = select_pcpu();
    list_push_tail(&pcpu->runqueue, &t->node);

    return t;
}

thread_t* create_user_thread(void (*entry)(void), proc_t *p, uint32_t priority, pcpu_t* pcpu) {
    if (!p) return NULL;

    vm_map_anon(p->vmspace, &user_stack_bottom, STACK_SIZE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER, 0);

    thread_t* t = kmalloc(sizeof(thread_t));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));

    t->tid = next_tid++;
    t->state = TASK_READY;
    t->priority = priority;
    t->proc = p;

    t->kstack = kmalloc(STACK_SIZE);
    if (!t->kstack) { kfree(t); return NULL; }
    memset(t->kstack, 0, STACK_SIZE);
    t->kstack_size = STACK_SIZE;

    uint32_t *stk = (uint32_t *)(t->kstack + STACK_SIZE);
    context_init(t, entry, stk, user_stack_bottom + STACK_SIZE); // Start user stack at the top

    list_push_tail(&p->threads, &t->proc_node);
    if (!pcpu) pcpu = select_pcpu();
    list_push_tail(&pcpu->runqueue, &t->node);
    pcpu->total_priority += priority;

    return t;
}

proc_t* create_process(const char* name) {
    proc_t *p = kmalloc(sizeof(proc_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p))    ;
    p->pid = next_pid++;
    p->ppid = 0; // For now, no parent-child relationships
    if (name) {
        // Copy name with safety
        strncpy(p->name, name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
    }

    list_init(&p->threads, 0);
    p->vmspace = vm_space_fork(kernel_vm_space); // Start with a copy of the kernel VM space
    
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

proc_t* fork_process(thread_t* t, int flags, proc_t** child_out) {
    if (!t || !t->proc) return NULL;

    proc_t* parent = t->proc;
    proc_t* child = kmalloc(sizeof(proc_t));
    if (!child) return NULL;
    memset(child, 0, sizeof(*child));

    child->pid = next_pid++;
    child->ppid = parent->pid;
    strncpy(child->name, parent->name, sizeof(child->name) - 1);
    child->name[sizeof(child->name) - 1] = '\0';

    child->vmspace = vm_space_fork(parent->vmspace);
    child->fd_table = fd_table_fork(parent->fd_table);

    list_init(&child->threads, 0);
    list_push_tail(&all_processes, &child->node);

    // Create a new thread for the child process that starts at the same entry point as the parent
    thread_t* child_thread = create_user_thread((void*)t->context->eip, child, t->priority, NULL);
    if (!child_thread) {
        list_remove(&child->node);
        kfree(child);
        return NULL;
    }

    if (child_out) *child_out = child;
    return child;
}

pcpu_t* select_pcpu() {
    pcpu_t* lowest = &pcpus[0];
    for (uint32_t i = 1; i < cpu_count; i++) {
        if (pcpus[i].total_priority < lowest->total_priority)
            lowest = &pcpus[i];
    }
    return lowest;
}

/* ---------------- Freeing / Reaping ---------------- */

// Free a single thread (assumes it's already removed from runqueue and proc list)
void free_thread(thread_t *t) {
    if (!t) return;
    pcpu_t* pcpu = PCPU_FROM_TASK(t);
    if (pcpu && t == pcpu->current_thread)
        PANIC("Attempted to free the current running thread!");

    list_remove(&t->proc_node); // Remove from process thread list
    list_remove(&t->node); // Remove from runqueue if still present
    pcpu->total_priority -= t->priority;

    if (t->proc && t->proc->threads.size == 0) {
        free_process(t->proc);
    }

    if (t->kstack) kfree(t->kstack);
    kfree(t);
}

void free_process(proc_t *p) {
    if (!p) return;

    printf("Freeing process %d (%s)\n", p->pid, p->name);

    list_remove(&p->node); // Remove from global process list

    if (p->vmspace)
        vm_space_destroy(p->vmspace);
    
    if (p->fd_table)
        fd_table_destroy(p->fd_table);

    kfree(p);
}

// Reap all zombie threads in the runqueue
static void reap_zombies(pcpu_t* pcpu) {
    if (!pcpu || !pcpu->runqueue.head) return;

    thread_t* t = (thread_t*)pcpu->runqueue.head;

    do {
        thread_t* next = (thread_t*)t->node.next;
        if (t != pcpu->current_thread && t->state == TASK_ZOMBIE) {
            free_thread(t);
        }
        t = next;
    } while (t && t != (thread_t*)pcpu->runqueue.head);
}

/* ---------------- Scheduling ---------------- */

thread_t* get_next_ready_thread(thread_t* prev) {
    thread_t* next = (thread_t*)prev->node.next;
    if (!next) PANIC("get_next_ready_thread: Current thread not in runqueue or runqueue is empty");

    thread_t* start = next;
    while (next->state != TASK_READY) {
        next = (thread_t*)next->node.next;
        if (!next || next == start) return NULL;
    }
    return next;
}

void thread_exit(registers_t* regs) {
    pcpu_t* pcpu = get_pcpu();
    pcpu->current_thread->state = TASK_ZOMBIE;
    schedule_from_irq(regs);
    PANIC("thread_exit: Returned from scheduler after marking thread as zombie, this should never happen!");
}

void schedule_from_irq(registers_t *regs) {
    pcpu_t *pcpu = get_pcpu();

    reap_zombies(pcpu);

    thread_t *prev = pcpu->current_thread;
    thread_t *next = get_next_ready_thread(prev);
    
    if (!next || next == prev) return;

    if (spin_trylock(&pcpu->scheduler_lock) != 0) return;

    if (prev->state == TASK_RUNNING)
      prev->state = TASK_READY;

    // Switch address space first so memory accesses to next's memory are correct
    vm_space_activate(next->proc->vmspace);

    // Update TSS.ESP0 so interrupts land on next kernel stack
    pcpu->tss.esp0 = (uint32_t)next->context; // We can use the thread's context pointer as the new kernel stack pointer since the context struct is at the top of the kernel stack

    // Set the CPU's notion of current thread
    pcpu->current_thread = next;

    // Load general purpose regs for next thread
    next->state = TASK_RUNNING;
    context_switch(&prev->context, next->context);
    spin_unlock(&pcpu->scheduler_lock);
}

/* ---------------- Wait Queue (for blocking I/O) ---------------- */

// Block the current thread and add it to a wait queue
void block_current_thread(list_t* wait_queue) {
    thread_t* current = PCPU_GET(current_thread);
    if (!current) return;
    
    // Change state to blocked
    current->state = TASK_BLOCKED;
    
    // Add to wait queue if provided
    if (wait_queue) {
        wait_node_t* wait_node = kmalloc(sizeof(wait_node_t));
        if (!wait_node) PANIC("block_current_thread: Out of memory");
        wait_node->thread = current;
        list_push_head(wait_queue, &wait_node->node);
    }
    
    // Trigger a software interrupt to invoke the scheduler from interrupt context
    // This ensures proper context saving/restoration via schedule_from_irq()
    // Timer interrupt is on vector 64 (0x40)
    __asm__ __volatile__("int $0x40");
}

// Wake up all threads in a wait queue
void wake_up_queue(list_t* wait_queue) {
    if (!wait_queue || !wait_queue->head) return;
    
    list_node_t* node = wait_queue->head;
    while (node) {
        wait_node_t* wait_node = WAIT_NODE_FROM_NODE(node);
        thread_t* t = wait_node->thread;
        if (t && t->state == TASK_BLOCKED) {
            t->state = TASK_READY;
        }
        
        list_node_t* to_free = node;
        node = node->next;
        list_remove(to_free);
        kfree(wait_node);
    }
}

/* ---------------- Debugging / Listing ---------------- */

void list_tasks() {
    printf("Process count: %u\n", next_pid - 1);
    printf("CPU   PID   TID   PPID  STATE    NAME\n");
    printf("==========================================\n");

    list_node_t* node = all_processes.head;
    while (node) {
        proc_t* p = PROC_FROM_NODE(node);
        printf("      %5u\n", p->pid);
        thread_t* t = THREAD_FROM_PROC_NODE(p->threads.head);
        while (t) {
            const char* state_str = "UNKNOWN";
            switch (t->state) {
                case TASK_RUNNING:  state_str = "RUNNING "; break;
                case TASK_READY:    state_str = "READY   "; break;
                case TASK_STOPPED:  state_str = "BLOCKED "; break;
                case TASK_SLEEPING: state_str = "SLEEPING"; break;
                case TASK_ZOMBIE:   state_str = "ZOMBIE  "; break;
            }
            printf("%5u %5u %5u %5u %s %s\n",
                PCPU_FROM_TASK(t)->pc_cpu_id,
                t->proc->pid,
                t->tid,
                t->proc->ppid,
                state_str,
                t->proc->name);

            t = THREAD_FROM_PROC_NODE(t->proc_node.next);
        }
        node = node->next;
    }
}

void list_pcpu_threads(pcpu_t* pcpu) {
    printf("CPU %u Runqueue:\n", pcpu->pc_cpu_id);
    printf("TID   PID   PPID  STATE    NAME\n");
    printf("====================================\n");

    list_node_t* node = pcpu->runqueue.head;
    if (!node) {
        printf("<empty>\n");
        return;
    }

    do {
        thread_t* t = (thread_t*)node;
        proc_t* p = t->proc;
        const char* state_str = "UNKNOWN";
        switch (t->state) {
            case TASK_RUNNING:  state_str = " RUNNING"; break;
            case TASK_READY:    state_str = "   READY"; break;
            case TASK_BLOCKED:  state_str = " BLOCKED"; break;
            case TASK_STOPPED:  state_str = " STOPPED"; break;
            case TASK_SLEEPING: state_str = "SLEEPING"; break;
            case TASK_ZOMBIE:   state_str = "  ZOMBIE"; break;
        }
        printf("%5u %5u %5u %s %s\n",
            t->tid,
            p->pid,
            p->ppid,
            state_str,
            p->name);

        node = node->next;
    } while (node && node != pcpu->runqueue.head);
}

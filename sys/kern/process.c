#include "process.h"
#include "elf.h"
#include "pcpu.h"
#include "fd.h"
#include "terminal.h"
#include "panic.h"

#include <machine/gdt.h>
#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <string.h>
#include <list.h>

#define STACK_SIZE 4096
#define USER_STACK_TOP 0xC0000000 // 3GB
#define USER_STACK_BOTTOM (USER_STACK_TOP - STACK_SIZE)

vaddr_t user_stack_bottom = USER_STACK_BOTTOM;


static list_t* all_processes = NULL;
proc_t* idle_process = NULL;

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

// Helpers to convert between list nodes and thread/process structures
#define PCPU_FROM_TASK(t) ((pcpu_t*)(t->node.list - offsetof(pcpu_t, runqueue)))
#define GET_RUNQUEUE (list_t*)(&PCPU_GET(runqueue) - offsetof(pcpu_t, runqueue))
#define PROC_FROM_NODE(node) ((node) ? (proc_t*)((uint8_t*)node - offsetof(proc_t, node)) : NULL)
#define THREAD_FROM_NODE(node) ((node) ? (thread_t*)((uint8_t*)node - offsetof(thread_t, node)) : NULL)
#define THREAD_FROM_PROC_NODE(node) ((node) ? (thread_t*)((uint8_t*)node - offsetof(thread_t, proc_node)) : NULL)
#define WAIT_NODE_FROM_NODE(node) ((node) ? (wait_node_t*)((uint8_t*)node - offsetof(wait_node_t, node)) : NULL)

void idle_task() {
    while (1) {
      asm volatile("hlt");
    }
}

/* ---------------- Runqueue Management ---------------- */

// Add a thread to the runqueue (circular linked list); handle empty list case
static void runqueue_add(pcpu_t* pcpu, thread_t *t) {
    list_push_tail(GET_RUNQUEUE, (list_node_t*)&t->node);
    pcpu->total_priority += t->priority;
}

static void runqueue_remove(pcpu_t* pcpu, thread_t *t) {
    list_node_t* node = list_find(GET_RUNQUEUE, &t->node);
    if (!node) return;
    list_remove(node);
    pcpu->total_priority -= t->priority;
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
void tasking_init(void) {
    all_processes = kmalloc(sizeof(list_t));
    if (!all_processes) PANIC("tasking_init: Out of memory");
    memset(all_processes, 0, sizeof(list_t));

    // bootstrap "idle" thread and process
    proc_t *p = create_process("idle");
    idle_process = p;

    thread_t *t = create_kernel_thread(idle_task, p, 0, &pcpus[0]);

    pcpus[0].current_thread = t;

    
    p->main_thread = t;
    proc_add_thread(p, t);
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

    uint8_t* stack = kmalloc(STACK_SIZE);
    if (!stack) { kfree(t); return NULL; }
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
    t->tcb.eip = (uint32_t)entry;
    t->tcb.esp0 = (uint32_t)(stack + STACK_SIZE);

    proc_add_thread(p, t);
    if (!pcpu) pcpu = select_pcpu();
    runqueue_add(pcpu, t);

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
    *(--stk) = (uint32_t)thread_exit; // Return address
    *(--stk) = (uint32_t)entry; // EIP
    *(--stk) = 0; // EBX
    *(--stk) = 0; // ESI
    *(--stk) = 0; // EDI
    *(--stk) = 0; // EBP

    t->tcb.esp =  (uint32_t)stk;
    t->tcb.eip = 0; // Will be set in user_eip
    t->tcb.esp0 = (uint32_t)(t->kstack + STACK_SIZE);

    // Set up user-mode stack frame
    t->tcb.user_esp = USER_STACK_TOP; // Initial ESP (user stack)
    t->tcb.user_eip = (uint32_t)entry;
    t->tcb.cs = 0x1B; // User mode code segment
    t->tcb.ds = 0x23; // User mode data segment
    t->tcb.es = 0x23;
    t->tcb.ss = 0x23; // User mode stack segment
    t->tcb.eflags = 0x202; // Enable interrupts

    proc_add_thread(p, t);
    if (!pcpu) pcpu = select_pcpu();
    runqueue_add(pcpu, t);

    return t;
}

proc_t* create_process(const char* name) {
    proc_t *p = kmalloc(sizeof(proc_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p))    ;
    p->pid = next_pid++;
    if (name) {
        // Copy name with safety
        strncpy(p->name, name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';
    }

    list_init(&p->threads, 0);
    p->vmspace = (vm_space_t*)kmalloc(sizeof(vm_space_t));
    if (!p->vmspace) {
        kfree(p);
        return NULL;
    }
    p->vmspace = vm_space_fork(kernel_vm_space); // Start with a copy of the kernel VM space
    
    // Create file descriptor table
    p->fd_table = fd_table_create();
    if (!p->fd_table) {
        kfree(p);
        return NULL;
    }
    
    // Initialize standard file descriptors
    fd_init_stdio(p);

    proc_list_add(p);
    return p;
}

pcpu_t* select_pcpu() {
    pcpu_t* lowest = &pcpus[0];
    for (uint32_t i = 1; i < cpu_count; i++) {
        if (pcpus[i].total_priority < lowest->total_priority)
            lowest = &pcpus[i];
    }
    return lowest;
}

/* ---------------- Current Process/Thread Helpers ---------------- */

thread_t* get_current_thread(void) {
    pcpu_t* pcpu = get_current_pcpu();
    return pcpu ? pcpu->current_thread : NULL;
}

proc_t* get_current_process(void) {
    thread_t* thread = get_current_thread();
    return thread ? thread->proc : NULL;
}

/* ---------------- Freeing / Reaping ---------------- */

// Free a single thread (assumes it's already removed from runqueue and proc list)
void free_thread(pcpu_t* pcpu, thread_t *t) {
    if (!t) return;

    if (pcpu && t == pcpu->current_thread)
        PANIC("Attempted to free the current running thread!");

    if (t->kstack) kfree(t->kstack);    
    kfree(t);
}

void free_process(proc_t *p) {
    if (!p) return;

    proc_list_remove(p);

    // Remove all threads
    while(p->threads.head) {
        list_node_t* node = list_pop_head(&p->threads);
        thread_t* t = THREAD_FROM_PROC_NODE(node);
        if (!t) continue;

        pcpu_t* pcpu = PCPU_FROM_TASK(t);
        if (pcpu) runqueue_remove(pcpu, t);
        free_thread(pcpu, t);
    }    

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
            runqueue_remove(pcpu, t);

            if (t->proc) {
                proc_remove_thread(t->proc, t);
                if (t->proc->threads.head == NULL)
                    free_process(t->proc);
            }
            free_thread(pcpu, t);
        }
        t = next;
    } while (t && t != (thread_t*)pcpu->runqueue.head);
}

/* ---------------- Scheduling ---------------- */

thread_t* get_next_ready_thread(thread_t* prev) {
    thread_t* next = (thread_t*)prev->node.next;
    if (!next) return NULL;

    thread_t* start = next;
    while (next->state != TASK_READY) {
        next = (thread_t*)next->node.next;
        if (!next || next == start) return NULL;
    }
    return next;
}



void thread_exit(registers_t* regs) {
    pcpu_t* pcpu = get_current_pcpu();
    if (!pcpu) PANIC("thread_exit: No current CPU!");
    pcpu->current_thread->state = TASK_ZOMBIE;
    schedule_from_irq(regs);
}

void schedule_from_irq(registers_t *regs) {
    pcpu_t *pcpu = get_current_pcpu();
    if (!pcpu) return;

    reap_zombies(pcpu);

    thread_t *prev = pcpu->current_thread;
    thread_t *next = get_next_ready_thread(prev);
    
    if (!next || next == prev) return;

    // --- Save current context into prev->tcb ---
    if (prev && prev->state != TASK_ZOMBIE) {
        prev->tcb.eax = regs->eax;
        prev->tcb.ebx = regs->ebx;
        prev->tcb.ecx = regs->ecx;
        prev->tcb.edx = regs->edx;
        prev->tcb.esi = regs->esi;
        prev->tcb.edi = regs->edi;
        prev->tcb.ebp = regs->ebp;
        
        if ((regs->cs & 0x3) == 3) {
            // From user mode
            prev->tcb.user_eip = regs->eip;
            prev->tcb.user_esp = regs->userEsp;
            prev->tcb.cs       = regs->cs;
            prev->tcb.ss       = regs->ss;
            prev->tcb.eflags   = regs->eflags;
        } else {
            // Interrupted from kernel mode.
            prev->tcb.user_eip = 0;
            prev->tcb.eip = regs->eip;
            prev->tcb.esp = regs->esp;
        }

        if (prev->state == TASK_RUNNING) prev->state = TASK_READY;
    }

    // --- Prepare next thread context ---

    // Switch address space first so memory accesses to next's memory are correct
    vm_space_activate(next->proc->vmspace);

    // Update TSS.ESP0 so interrupts land on next kernel stack
    pcpu->tss.esp0 = next->tcb.esp0;

    // Set the CPU's notion of current thread
    pcpu->current_thread = next;

    // Load general purpose regs for next thread
    regs->eax = next->tcb.eax;
    regs->ebx = next->tcb.ebx;
    regs->ecx = next->tcb.ecx;
    regs->edx = next->tcb.edx;
    regs->esi = next->tcb.esi;
    regs->edi = next->tcb.edi;
    regs->ebp = next->tcb.ebp;

    if (next->tcb.user_eip != 0) {
        // Resume into user mode (commonly the case)
        regs->eip = next->tcb.user_eip;
        regs->cs  = next->tcb.cs;
        regs->userEsp = next->tcb.user_esp;
        regs->ss  = next->tcb.ss;
        regs->eflags = next->tcb.eflags | (1 << 9);
        regs->ds = regs->es = regs->fs = regs->gs = 0x23;
    } else {
        // Resume into kernel mode (kernel thread)
        regs->eip = next->tcb.eip;
        regs->esp = next->tcb.esp;
        regs->cs  = 0x08;
        regs->eflags = next->tcb.eflags | (1 << 9);
        regs->ds = regs->es = regs->fs = regs->gs = 0x10;
    }
    next->state = TASK_RUNNING;
}

/* ---------------- Wait Queue (for blocking I/O) ---------------- */

// Block the current thread and add it to a wait queue
void block_current_thread(list_t* wait_queue) {
    thread_t* current = get_current_thread();
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
    printf("CPU   PID   TID   PPID  STATE    NAME\n");
    printf("==========================================\n");

    list_node_t* node = all_processes->head;
    while (node) {
        proc_t* p = PROC_FROM_NODE(node);
        thread_t* t = p->main_thread;
        while (t) {
            const char* state_str = "UNKNOWN";
            switch (t->state) {
                case TASK_RUNNING:  state_str = "RUNNING "; break;
                case TASK_READY:    state_str = "READY   "; break;
                case TASK_STOPPED:  state_str = "BLOCKED "; break;
                case TASK_SLEEPING: state_str = "SLEEPING"; break;
                case TASK_ZOMBIE:   state_str = "ZOMBIE  "; break;
            }
            printf("%5u %5u %5u %5u %s %s %x\n",
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
            p->main_thread->proc->pid,
            state_str,
            p->name);

        node = node->next;
    } while (node && node != pcpu->runqueue.head);
}
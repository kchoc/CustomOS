#include "kernel/descriptors/gdt.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vmspace.h"
#include "kernel/memory/vm.h"
#include "kernel/process/process.h"
#include "kernel/process/elf.h"
#include "kernel/process/cpu.h"
#include "kernel/terminal.h"
#include "kernel/panic.h"
#include "types/string.h"
#include "types/list.h"
#include <stdint.h>

#define STACK_SIZE 4096
#define USER_STACK_TOP 0xC0000000 // 3GB
#define USER_STACK_BOTTOM (USER_STACK_TOP - STACK_SIZE)


static list_t* all_processes = NULL;
proc_t* idle_process = NULL;

static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

// Helpers to convert between list nodes and thread/process structures
#define CPU_FROM_TASK(t) ((cpu_t*)(t->node.list - offsetof(cpu_t, runqueue)))
#define PROC_FROM_NODE(node) ((proc_t*)((uint8_t*)node - offsetof(proc_t, node)))
#define THREAD_FROM_NODE(node) ((thread_t*)((uint8_t*)node - offsetof(thread_t, node)))
#define THREAD_FROM_PROC_NODE(node) ((thread_t*)((uint8_t*)node - offsetof(thread_t, proc_node)))

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
void tasking_init(void) {
    all_processes = kmalloc(sizeof(list_t));
    if (!all_processes) PANIC("tasking_init: Out of memory");
    memset(all_processes, 0, sizeof(list_t));

    // bootstrap "idle" thread and process
    proc_t *p = create_process("idle");
    idle_process = p;

    vm_space_t* old = vm_space_switch(p->vmspace);

    thread_t *t = create_kernel_thread(idle_task, p, 0, &cpus[0]);

    cpus[0].current_thread = t;

    
    p->main_thread = t;
    proc_add_thread(p, t);

    vm_space_switch(old);
}

thread_t* create_kernel_thread(void (*entry)(void), proc_t *p, uint32_t priority, cpu_t* cpu) {
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
    t->tcb.cr3 = (uint32_t)p->vmspace->page_directory;

    proc_add_thread(p, t);
    if (!cpu) cpu = select_cpu();
    runqueue_add(cpu, t);

    return t;
}

thread_t* create_user_thread(void (*entry)(void), proc_t *p, uint32_t priority, cpu_t* cpu) {
    if (!p) return NULL;

    vm_space_map(p->vmspace, (void *)USER_STACK_BOTTOM, 0, STACK_SIZE, VM_PROT_READWRITE | VM_PROT_USER, 0);

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
    t->tcb.cr3 =  (uint32_t)p->vmspace->page_directory;

    // Set up user-mode stack frame
    t->tcb.user_esp = USER_STACK_TOP; // Initial ESP (user stack)
    t->tcb.user_eip = (uint32_t)entry;
    t->tcb.cs = 0x1B; // User mode code segment
    t->tcb.ds = 0x23; // User mode data segment
    t->tcb.es = 0x23;
    t->tcb.ss = 0x23; // User mode stack segment
    t->tcb.eflags = 0x202; // Enable interrupts

    proc_add_thread(p, t);
    if (!cpu) cpu = select_cpu();
    runqueue_add(cpu, t);

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

    if (cpu && t == cpu->current_thread)
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

        cpu_t* cpu = CPU_FROM_TASK(t);
        if (cpu) runqueue_remove(cpu, t);
        if (t->kstack) kfree(t->kstack);
        kfree(t);
    }    

    if (p->vmspace)
        vm_space_destroy(p->vmspace);

    kfree(p);
}

// Reap all zombie threads in the runqueue
static void reap_zombies(cpu_t* cpu) {
    if (!cpu) return;

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

void thread_exit(registers_t* regs) {
    cpu_t* cpu = get_current_cpu();
    if (!cpu) PANIC("thread_exit: No current CPU!");
    cpu->current_thread->state = TASK_ZOMBIE;
    schedule_from_irq(regs);
}

void schedule_from_irq(registers_t *regs) {
    cpu_t *cpu = get_current_cpu();
    if (!cpu) return;

    reap_zombies(cpu);

    thread_t *prev = cpu->current_thread;
    thread_t *next = (thread_t*)prev->node.next;
    
    if (!next || next == prev)
        return;

    // --- Save current context into prev->tcb ---
    if (prev) {
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
        }

        prev->tcb.eax = regs->eax;
        prev->tcb.ebx = regs->ebx;
        prev->tcb.ecx = regs->ecx;
        prev->tcb.edx = regs->edx;
        prev->tcb.esi = regs->esi;
        prev->tcb.edi = regs->edi;
        prev->tcb.ebp = regs->ebp;
        prev->state = TASK_READY;
    }

    // --- Prepare next thread context ---
    // Switch address space first so memory accesses to next's memory are correct
    vm_space_switch(next->proc->vmspace);

    // Update TSS.ESP0 so interrupts land on next kernel stack
    cpu->tss.esp0 = next->tcb.esp0;

    // Set the CPU's notion of current thread
    cpu->current_thread = next;

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
        regs->userEsp = next->tcb.esp;
        regs->cs  = 0x08;
        regs->eflags = next->tcb.eflags | (1 << 9);
        regs->ds = regs->es = regs->fs = regs->gs = 0x10;
    }
    next->state = TASK_RUNNING;
}

void schedule() {
    cpu_t* cpu = get_current_cpu();
    if (!cpu) PANIC("schedule: No current CPU!");

    reap_zombies(cpu);

    thread_t *prev = cpu->current_thread;
    if (!prev) PANIC("No current thread to schedule from!");

    thread_t *next = (thread_t*)prev->node.next;
    if (!next) next = (thread_t*)cpu->runqueue.head;

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
    printf("CPU   PID   TID   PPID  STATE    NAME\n");
    printf("==========================================\n");

    list_node_t* node = all_processes->head;
    while (node) {
        proc_t* p = (proc_t*)node;
        thread_t* t = PROC_FROM_NODE(node)->main_thread;
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
                CPU_FROM_TASK(t)->apic_id,
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

void list_cpu_threads(cpu_t* cpu) {
    printf("CPU %u Runqueue:\n", cpu->apic_id);
    printf("TID   PID   PPID  STATE    NAME\n");
    printf("====================================\n");

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
            case TASK_RUNNING:  state_str = " RUNNING"; break;
            case TASK_READY:    state_str = "   READY";   break;
            case TASK_STOPPED:  state_str = " BLOCKED"; break;
            case TASK_SLEEPING: state_str = "SLEEPING"; break;
            case TASK_ZOMBIE:   state_str = "  ZOMBIE";  break;
        }
        printf("%5u %5u %5u %s %s\n",
            t->tid,
            p->pid,
            p->main_thread->proc->pid,
            state_str,
            p->name);

        node = node->next;
    } while (node && node != cpu->runqueue.head);
}
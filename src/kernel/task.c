#include "kernel/task.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/gdt.h"
#include "kernel/filesystem/fs.h"
#include "kernel/memory/page.h"
#include "kernel/terminal.h"
#include <stdint.h>

#define STACK_SIZE 4096
#define MAX_PROGRAM_SIZE (1024 * 1024) // 1 MB
#define PROGRAM_ENTRY (void (*)(void))0x00001000

task_t* current_task = 0;
static task_t* task_list = 0;
static uint32_t next_pid = 0;

void tasking_init() {
    task_t* kernel = kmalloc(sizeof(task_t));
    tcb_t* tcb = kmalloc(sizeof(tcb_t));
    kernel->id = 0;
    tcb->esp = 0;
    tcb->esp0 = 0;
    tcb->cr3 = get_current_page_table_phys();

    kernel->tcb = tcb;
    kernel->next = kernel;

    current_task = kernel;
    task_list = kernel;
}

task_t* create_task(void (*entry)(void)) {
    task_t *task = kmalloc(sizeof(task_t));
    tcb_t *tcb = kmalloc(sizeof(tcb_t));

    uint8_t *stack = kmalloc(STACK_SIZE);
    uint32_t *esp = (uint32_t *)(stack + STACK_SIZE);

    *(--esp) = (uint32_t)entry; // Fake return address (EIP)
    *(--esp) = 0; // EBP
    *(--esp) = 0; // EDI
    *(--esp) = 0; // ESI
    *(--esp) = 0; // EBX

    tcb->esp = esp;
    tcb->esp0 = (uint32_t)(stack + STACK_SIZE);
    tcb->cr3 = new_task_page_table();

    task->id = next_pid++;
    task->tcb = tcb;
    task->next = task_list;
    task_list = task;

    return task;
}

void create_task_from_binary(const char* path) {
    task_t *task = create_task(PROGRAM_ENTRY);
    page_table_load(task->tcb->cr3);
    // Read the binary file
    if (fat16_read_file_to_buffer(path, (uint8_t *)PROGRAM_ENTRY, MAX_PROGRAM_SIZE) != 0)
        return;
}

void switch_task() {
    if (!current_task || !current_task->next) return;

    task_t *next = current_task->next ? current_task->next : task_list;

    if (next == current_task) return; // Only one task

    write_tss(5, 0x10, next->tcb->esp0);
    switch_to(next->tcb);
}

int exit_task(task_t *task) {
    
}

void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack) {
    asm volatile (
        "cli\n"
        "mov $0x23, %%ax\n"     // USER_DS
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        "pushl $0x23\n"         // SS
        "pushl %0\n"            // ESP
        "pushf\n"
        "pushl $0x1B\n"         // CS
        "pushl %1\n"            // EIP
        "iret\n"
        :
        : "r"(user_stack), "r"(entry_point)
        : "ax"
    );
}

// void switch_to_kernel_mode() {
//     asm volatile (
//         "cli\n"
//         "mov %0, %%esp\n" // Switch to kernel stack
//         "jmp kernel_entry\n"
//         :
//         : "r"(current_task->TCB->ESP0)
//     );
// }


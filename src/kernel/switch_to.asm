;C declaration:
;   void switch_tasks(thread_control_block *next_thread);
;
;WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns

extern current_task
extern terminal_test
extern terminal_print_register_value
extern tss

%define TCB_ESP     0
%define TCB_CR3     4
%define TCB_ESP0    8

%define TSS_ESP0    4

[GLOBAL switch_to]
switch_to:

    ;Save previous task's state

    ;Notes:
    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    ;  EIP is already saved on the stack by the caller's "CALL" instruction
    ;  The task isn't able to change CR3 so it doesn't need to be saved
    ;  Segment registers are constants (while running kernel code) so they don't need to be saved

    push ebx
    push esi
    push edi
    push ebp

    mov edi,[current_task]
    mov edi, [edi]              ; edi = address of the previous task's TCB
    mov [edi+TCB_ESP],esp       ; Save ESP for previous task's kernel stack in the thread's TCB

    ;Load next task's state
    mov esi,[esp+20]            ; esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_task], esi     ; Current task's TCB is the next task TCB

    mov esp,[esi+TCB_ESP]       ; Load ESP for next task's kernel stack from the thread's TCB
    mov eax,[esi+TCB_CR3]       ; eax = address of page directory for next task
    mov ebx,[esi+TCB_ESP0]      ; ebx = address for the top of the next task's kernel stack

    mov [tss+TSS_ESP0],ebx      ; Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov cr3,eax                 ; load the next task's page directory

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret                           ;Load next task's EIP from its kernel stack

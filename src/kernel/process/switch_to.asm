; void switch_to(struct pcb *prev, struct pcb *next);
; cdecl: [esp+4] = prev, [esp+8] = next
; Requirements:
;   - Interrupts disabled by caller.
;   - Each thread’s kernel stack is set up so that:
;       [ ... caller-saved ... ]
;       [ return EIP ]          <-- ret will jump here
;       [ ebx ] [ esi ] [ edi ] [ ebp ]  <-- these will be popped by this routine
;   - tss symbol points at your TSS, and TSS_ESP0 offset is correct.

extern tss

%define PCB_ESP      0
%define PCB_CR3      12
%define PCB_ESP0     16
%define TSS_ESP0     4

global switch_to
switch_to:
    ; Get args early (before we touch the stack)
    mov     eax, [esp + 4]        ; eax = prev pcb*
    mov     edx, [esp + 8]        ; edx = next pcb*

    ; Fast path: prev == next → nothing to do
    cmp     eax, edx
    je      .done

    ; Save callee-saved registers on current (prev) stack
    push    ebx
    push    esi
    push    edi
    push    ebp

    ; Save current kernel stack pointer into prev->esp
    mov     [eax + PCB_ESP], esp

    ; Load next context
    mov     esp, [edx + PCB_ESP]      ; switch to next's kernel stack
    mov     ecx, [edx + PCB_CR3]      ; next->cr3
    mov     ebx, [edx + PCB_ESP0]     ; next->esp0

    ; Update TSS.ESP0 so user->kernel transitions land on next's kernel stack
    mov     [tss + TSS_ESP0], ebx

    ; Switch address space
    mov     cr3, ecx

    ; Restore callee-saved regs from next's stack and jump to its saved EIP
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret

.done:
    ret

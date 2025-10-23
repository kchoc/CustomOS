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

%define TCB_ESP         0
%define TCB_EIP         4
%define TCB_CR3         8
%define TCB_ESP0        12

%define TCB_USER_EIP    16
%define TCB_USER_ESP    20
%define TCB_CS          24
%define TCB_DS          28
%define TCB_ES          32
%define TCB_SS          36
%define TCB_EFLAGS      40

%define TSS_ESP0     4

global switch_to
switch_to:
    ; -------------------------------
    ; Input: prev (pcb*), next (pcb*)
    ; -------------------------------
    mov     eax, [esp + 4]        ; eax = prev pcb*
    mov     edx, [esp + 8]        ; edx = next pcb*

    ; Fast path: prev == next → nothing to do
    cmp     eax, edx
    je      .done

    ; -------------------------------
    ; Save callee-saved kernel registers into prev->esp
    ; -------------------------------
    push    ebx
    push    esi
    push    edi
    push    ebp

    mov     [eax + TCB_ESP], esp

    ; -------------------------------
    ; Load next context
    ; -------------------------------
    mov     esp, [edx + TCB_ESP]      ; switch to next's kernel stack
    mov     ecx, [edx + TCB_CR3]      ; next->cr3
    mov     ebx, [edx + TCB_ESP0]     ; next->esp0

    mov     [tss + TSS_ESP0], ebx
    mov     cr3, ecx

    ; -------------------------------
    ; Restore callee-saved kernel registers from next's stack
    ; -------------------------------
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx

    ; -------------------------------
    ; Check if we need to switch to user mode
    ; -------------------------------
    mov     eax, [edx + TCB_USER_EIP]
    test    eax, eax
    jz      .done            ; If USER_EIP is 0, stay in kernel mode

    ; -------------------------------
    ; Switch to user mode
    ; -------------------------------
    push    dword   [edx + TCB_SS]
    push    dword   [edx + TCB_USER_ESP]
    push    dword   [edx + TCB_EFLAGS]
    push    dword   [edx + TCB_CS]
    push    dword   [edx + TCB_USER_EIP]
    
    iret                       ; Switch to user mode

.done:
    ret

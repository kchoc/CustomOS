#include "machine/asmmacros.h"
#include "machine/genassym.h"

ENTRY(lgdt)
	/* reload the descriptor table */
	movl	4(%esp),%eax
	lgdt	(%eax)

	/* flush the prefetch q */
	jmp	1f
	nop
1:
	/* reload "stale" selectors */
	movl	$KDSEL,%eax
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%gs
	movl	%eax,%ss
	movl	$KPSEL,%eax
	movl	%eax,%fs

	/* reload code selector by turning return into intersegmental return */
	movl	(%esp),%eax
	pushl	%eax
	movl	$KCSEL,4(%esp)
	lret
END(lgdt)

# cdecl for load_tss:
# void load_tss(uint16_t tss_selector);
ENTRY(load_tss)
    movw    4(%esp), %ax      # Load the TSS selector from the stack into ax
    ltr     %ax              # Load the TSS selector into the task registers
    
    ret                     # Return from the function
END(load_tss)

ENTRY(context_switch)

    # Arguments (cdecl):
    # 4(%esp)  = context_t **old
    # 8(%esp)  = context_t *new

    movl    4(%esp), %eax      # eax = old
    movl    8(%esp), %edx      # edx = new

    # Save callee-saved registers
    pushl   %ebp
    pushl   %ebx
    pushl   %esi
    pushl   %edi

    # Save current stack pointer into *old
    movl    %esp, (%eax)

    # Switch to new stack
    movl    %edx, %esp

    # Restore registers from new context
    popl    %edi
    popl    %esi
    popl    %ebx
    popl    %ebp

    # Return to new EIP
    ret

END(context_switch)

.extern spin_unlock

ENTRY(start_thread)
    # Must unlock the scheduler lock before starting the thread's entry pointer
    # We can get the pcpu pointer from the gs segment register, which is set up to point to the per-cpu data area
    movl    %gs:0, %eax  # Load the pcpu pointer from gs:0 into eax
    addl    $SCHEDULER_LOCK_OFFSET, %eax  # Add the offset of the scheduler lock to get its address
   
    pushl   %eax      # Push the address of the scheduler lock onto the stack for spin_unlock
    call    spin_unlock  # Unlock the scheduler lock
    popl    %eax      # Clean up the stack after the call to spin_unlock

    popl    %eax      # Pop the thread's entry point into eax
    popl    %ebx      # Pop the user esp or 0 for kernel thread into ebx

    # Set up trap frame for the new thread
    # Check if it's a kernel thread or user thread
    cmp     $0, %ebx
    je      .kernel_thread

    # User thread setup
    # Set up user mode segment selectors
    movl    $UDSEL, %ecx
    movw    %cx, %ds
    movw    %cx, %es
    
    # Set up user stack and ss
    pushl   $UDSEL   # User data segment selectors
    pushl   %ebx     # User stack pointer
    pushl   $0x200   # EFLAGS with interrupts enabled
    pushl   $UCSEL   # User code segment selector
    pushl   %eax     # Push the thread's entry point as the return address

    iret                # Return to user mode

.kernel_thread:
    # Kernel thread setup
    # Set up kernel mode segment selectors
    movl    $KDSEL, %ecx
    movw    %cx, %ds
    movw    %cx, %es
    movw    %cx, %fs
    movw    %cx, %gs
    
    # Set up kernel stack and ss
    pushl   $0x200   # EFLAGS with interrupts enabled
    pushl   $KCSEL   # Kernel code segment selector
    pushl   %eax     # Push the thread's entry point as the return address

    iret                # Return to kernel mode

END(start_thread)


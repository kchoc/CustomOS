#ifndef x86_I386_INTERRUPTS_H
#define x86_I386_INTERRUPTS_H

int  i386_idt_init();
void i386_interrupt_enable();
void i386_interrupt_disable();

#endif // x86_I386_INTERRUPTS_H

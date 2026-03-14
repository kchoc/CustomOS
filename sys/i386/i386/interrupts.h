#ifndef I386_INTERRUPTS_H
#define I386_INTERRUPTS_H

int  i386_idt_init();
void i386_interrupt_enable();
void i386_interrupt_disable();

#endif // I386_INTERRUPTS_H

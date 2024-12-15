#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// ISR handler definition
void isr_default_handler(void);
void keyboard_isr_handler(void);

#endif // ISR_H
#ifndef x86_IO_H
#define x86_IO_H

#include <kern/types.h>

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

#endif // x86_IO_H

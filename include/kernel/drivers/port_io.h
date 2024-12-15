#ifndef PORT_IO_H
#define PORT_IO_H

#include <stdint.h>

// Read a byte from a port
uint8_t inb(uint16_t port);

// Write a byte to a port
void outb(uint16_t port, uint8_t data);

// Read a word (2 bytes) from a port
uint16_t inw(uint16_t port);

// Write a word (2 bytes) to a port
void outw(uint16_t port, uint16_t data);

#endif // PORT_IO_H
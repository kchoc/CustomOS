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

// Write a block of words to a port
void outsw(uint16_t port, const void* addr, int count);

// Read a block of words from a port
void insw(uint16_t port, void* addr, int count);

void ide_write_sector(uint32_t lba, const uint8_t* data);
void ide_read_sector(uint32_t lba, uint8_t* buffer);

#endif // PORT_IO_H
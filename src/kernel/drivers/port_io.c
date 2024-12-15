#include "kernel/drivers/port_io.h"

// Read a byte from a port
uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// Write a byte to a port
void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" :: "a"(data), "Nd"(port));
}

// Read a word (2 bytes) from a port
uint16_t inw(uint16_t port) {
    uint16_t data;
    asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// Write a word (2 bytes) to a port
void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1" :: "a"(data), "Nd"(port));
}
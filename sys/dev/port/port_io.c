#include "port_io.h"

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

// Read a double word (4 bytes) from a port
uint32_t inl(uint16_t port) {
    uint32_t data;
    asm volatile("inl %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// Write a double word (4 bytes) to a port
void outl(uint16_t port, uint32_t data) {
    asm volatile("outl %0, %1" :: "a"(data), "Nd"(port));
}

void outsw(uint16_t port, const void* addr, int count) {
    __asm__ volatile ("rep outsw"
                      : "+S"(addr), "+c"(count)
                      : "d"(port));
}

void insw(uint16_t port, void* addr, int count) {
    __asm__ volatile ("rep insw"
                      : "+D"(addr), "+c"(count)
                      : "d"(port): "memory");
}

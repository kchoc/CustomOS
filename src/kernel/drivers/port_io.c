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

void ide_write_sector(uint32_t lba, const uint8_t* data) {
    while (inb(0x1F7) & 0x80);
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x30);

    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    outsw(0x1F0, data, 256);

    while (inb(0x1F7) & 0x80);
    while (inb(0x1F7) & 0x08);

    outb(0x1F7, 0xE7);
}

void ide_read_sector(uint32_t lba, uint8_t* buffer) {
    while (inb(0x1F7) & 0x80);
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x20);

    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    insw(0x1F0, buffer, 256);
}

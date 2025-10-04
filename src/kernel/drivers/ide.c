#include "kernel/drivers/ide.h"
#include "kernel/drivers/port_io.h"

void ide_write_sector(uint32_t lba, const uint8_t* data) {
	// Wait until the drive is ready
    while (inb(0x1F7) & 0x80);

    // Send the write command and parameters
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x30);

    // Wait for the drive to be ready to accept data
    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    // Write the sector data
    outsw(0x1F0, data, 256);

    // Wait for the write to complete
    while (inb(0x1F7) & 0x80);
    while (inb(0x1F7) & 0x08);

    // Send cache flush command
    outb(0x1F7, 0xE7);
}

void ide_read_sector(uint32_t lba, uint8_t* buffer) {
	// Wait until the drive is ready
    while (inb(0x1F7) & 0x80);

    // Send the read command and parameters
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x20);

    // Wait for the drive to be ready to send data
    while (inb(0x1F7) & 0x80);
    while (!(inb(0x1F7) & 0x08));

    // Read the sector data
    insw(0x1F0, buffer, 256);
}
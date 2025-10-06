#include "kernel/time/pit.h"
#include "kernel/drivers/port_io.h"

#define PIT_FREQUENCY 1193182  // PIT input clock frequency
#define PIT_PORT_CMD 0x43
#define PIT_PORT_CH0 0x40
#define PIT_MODE_ONESHOT 0x30  // Channel 0, lobyte/hibyte, mode 0 (interrupt on terminal count), binary mode


void delay_us(int us) {
    uint32_t count = (PIT_FREQUENCY * us) / 1000000;
    if (count == 0) count = 1;

    // Send command byte
    outb(PIT_PORT_CMD, PIT_MODE_ONESHOT);
    // Send low byte first
    outb(PIT_PORT_CH0, count & 0xFF);
    // Then high byte
    outb(PIT_PORT_CH0, (count >> 8) & 0xFF);

    // Wait until counter reaches zero
    while (inb(PIT_PORT_CH0) != 0) {
        asm volatile("pause");
    }
}

void delay_ms(int ms) {
    while (ms--) delay_us(1000);
}

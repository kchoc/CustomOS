#ifndef X86_IOAPIC_H
#define X86_IOAPIC_H

#include <inttypes.h>
#include <stdint.h>

typedef struct ioapic {
	uint32_t regsel;	// 0x00
	uint32_t reserved[3];
	uint32_t data;		// 0x10
} ioapic_t;

typedef struct ioapic_redir_entry {
	uint8_t vector : 8;        		// Interrupt vector (bits 0-7)
	uint8_t delivery_mode : 3; 		// Delivery mode (bits 8-10)
	uint8_t dest_mode : 1;     		// Destination mode (bit 11)
	uint8_t delivery_status : 1; 	// Delivery status (bit 12)
	uint8_t polarity : 1;     		// Polarity (bit 13)
	uint8_t remote_irr : 1;   		// Remote IRR (bit 14)
	uint8_t trigger_mode : 1; 		// Trigger mode (bit 15)
	uint8_t mask : 1;        		// Interrupt mask (bit 16)
	uint8_t reserved : 7;   		// Reserved (bits 17-23)
	uint8_t reserved2 : 8;   		// Reserved (bits 24-31)
	uint8_t dest;          			// Destination field (bits 56-63)
} ioapic_redir_entry_t;

#define IOAPIC_BASE 0xFEC00000

void ioapic_init();
void ioapic_check();
void ioapic_enable_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id);

#endif // X86_IOAPIC_H

#ifndef X86_LAPIC_H
#define X86_LAPIC_H

#include <stdint.h>

typedef struct lapic_register {
	uint32_t value;
	uint32_t reserved[3];
} lapic_register_t;

typedef struct lapic {
	lapic_register_t reserved0[2];		// 0x000 - 0x01F	reserved
	lapic_register_t id;				// 0x020			APIC ID (bits 31:24)
	lapic_register_t version;			// 0x030			APIC Version
	lapic_register_t reserved1[4];		// 0x040 - 0x07F	reserved
	lapic_register_t tpr;				// 0x080			Task Priority Register
	lapic_register_t apr;				// 0x090			Arbitrary Priority Register
	lapic_register_t ppr;				// 0x0A0			Processor Priority Register
	lapic_register_t eoi;				// 0x0B0			End Of Interrupt Register
	lapic_register_t rrd;				// 0x0C0			Remote Read Register (for xAPIC, read returns 0; for x2APIC, read returns data from remote APIC)
	lapic_register_t ldr;				// 0x0D0			Logical Destination Register (for xAPIC, bits 31:24 contain the logical APIC ID; for x2APIC, this register is reserved and returns 0)
	lapic_register_t dfr;				// 0x0E0			APIC Destination Format Register (for xAPIC, bits 28:31 indicate the destination mode; for x2APIC, this register is reserved and returns 0)
	lapic_register_t svr;				// 0x0F0			Spurious Interrupt Vector Register
	lapic_register_t isr[8];			// 0x100 - 0x17F	In-Service Register (ISR) bits
	lapic_register_t tmr[8];			// 0x180 - 0x1FF	Trigger Mode Register (TMR) bits
	lapic_register_t irr[8];			// 0x200 - 0x27F	Interrupt Request Register (IRR) bits
	lapic_register_t error_status;		// 0x280			Error Status Register (ESR)
	lapic_register_t reserved2[6];		// 0x290 - 0x2EF	reserved
	lapic_register_t lvt_cmci;			// 0x2F0			LVT Corrected Machine Check Interrupt Register
	lapic_register_t icr_low;			// 0x300			Interrupt Command Register (ICR) low dword
	lapic_register_t icr_high;			// 0x310			Interrupt Command Register (ICR) high dword
	lapic_register_t lvt_timer;			// 0x320			LVT Timer Register
	lapic_register_t lvt_thermal;		// 0x330			LVT Thermal Sensor Register
	lapic_register_t lvt_perf;			// 0x340			LVT Performance Monitoring Counters Register
	lapic_register_t lvt_lint0;			// 0x350			LVT LINT0 Register
	lapic_register_t lvt_lint1;			// 0x360			LVT LINT1 Register
	lapic_register_t lvt_error;			// 0x370			LVT Error Register
	lapic_register_t timer_initial; 	// 0x380			Initial Count Register for Timer
	lapic_register_t timer_current; 	// 0x390			Current Count Register for Timer
	lapic_register_t reserved3[4];		// 0x3A0 - 0x3DF	reserved
	lapic_register_t timer_divide;		// 0x3E0			Divide Configuration Register for Timer
} lapic_t;

typedef struct lapic_timer {
	uint32_t initial_count;	// Initial count value for the timer
	uint32_t current_count;	// Current count value (read-only)
	uint32_t divide_config;	// Divide configuration for the timer
	uint32_t vector;		// Interrupt vector number for the timer
} lapic_timer_t;

#endif // X86_LAPIC_H

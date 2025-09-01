#ifndef BDA_H
#define BDA_H

#include <stdint.h>

typedef struct bios_data_area {
	uint16_t com_ports[4]; 		// 0x400, COM1..COM4
	uint16_t lpt_ports[3]; 		// 0x408, LPT1..LPT3
	uint16_t ebda_segment;   	// 0x40E
	uint16_t equipment_flags; 	// 0x410
	uint8_t  reserved1[1];  	// 0x412-0x412
	uint16_t memory_kb;    		// 0x413, size of base memory in KB
	uint8_t  reserved2[2];  	// 0x415-0x416, Reserved & BIOS Control Flags
	uint16_t keyboard_flags; 	// 0x417
	uint8_t  reserved3[1];  	// 0x419, alternate keypad entry
	uint16_t keyboard_buffer_head; // 0x43A
	uint16_t keyboard_buffer_tail; // 0x43B
	uint8_t  keyboard_buffer[32]; // 0x41A-0x439, keyboard buffer
	// ... (other BDA fields can be added as needed)
} bda_t;

typedef struct extended_bios_data_area {
	uint8_t  reserved1[4];      // 0x44E-0x451
	uint16_t diskette_controller_flags; // 0x452
	uint8_t  reserved2[10];     // 0x454-0x45D
	uint8_t  hard_disk_count;   // 0x45E
	uint8_t  reserved3[1];      // 0x45F
	uint16_t hard_disk_type[2]; // 0x460-0x463
	// ... (other EBDA fields can be added as needed)
} ebda_t;

extern bda_t* bda;
extern ebda_t* ebda;

void load_ebda();

#endif // BDA_H
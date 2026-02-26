#ifndef SYS_ASSYM_H
#define SYS_ASSYM_H

// Assembler Symbolic Constants
// This file is used to generate symbolic constants for use in assembly code. It is
// processed by the 'assym' tool, which extracts the defined constants and their values.

#if __ASSEMBLER__
	#define ASSYM(name, value) \
		.equ name, value
#else
	#define ASSYM(name, value) \
		enum { name = value }
#endif

#endif // SYS_ASSYM_H

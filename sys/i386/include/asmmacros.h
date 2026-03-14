#ifndef _I386_ASMMACROS_H_
#define _I386_ASMMACROS_H_

// Macros to link C and assembly code
#define ENTRY(name)       	\
	.globl name;          	\
	.type name, @function;	\
name:

#define END(name)          \
	.size name, .-name

#endif // _I386_ASMMACROS_H_

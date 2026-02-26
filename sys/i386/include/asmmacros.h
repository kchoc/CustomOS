#ifndef I386_ASMMACROS_H
#define I386_ASMMACROS_H

// Macros to link C and assembly code
#define ENTRY(name)       	\
	.globl name;          	\
	.type name, @function;	\
name:

#define END(name)          \
	.size name, .-name

#endif // I386_ASMMACROS_H

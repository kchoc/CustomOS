#ifndef X86_COMMON_GDT_H
#define X86_COMMON_GDT_H

#if defined(__i386__)
	#include "arch/x86/i386/gdt.h"
	typedef i386_gdt_t gdt_t;
	typedef i386_gdt_ptr_t gdt_ptr_t;
#else
	#error "Unsupported architecture for x86 common GDT"
#endif


#endif // X86_COMMON_GDT_H
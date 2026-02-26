#ifndef X86_COMMON_TSS_H
#define X86_COMMON_TSS_H

#if defined(__i386__)
	#include "arch/x86/i386/tss.h"
	typedef i386_tss_t tss_t;
#else
	#error "Unsupported architecture for x86 common TSS"
#endif


#endif // X86_COMMON_TSS_H
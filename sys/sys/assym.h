#ifndef SYS_ASSYM_H
#define SYS_ASSYM_H

#ifdef __GENASSYM__
#include <stdio.h>

#define ASSYM(name, value) \
	printf("#define %s %d\n", #name, (int)(value))
#endif

#endif // SYS_ASSYM_H

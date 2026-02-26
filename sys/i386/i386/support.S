#include "machine/asmmacros.h"
#include "machine/genassym.h"

ENTRY(lgdt)
	/* reload the descriptor table */
	movl	4(%esp),%eax
	lgdt	(%eax)

	/* flush the prefetch q */
	jmp	1f
	nop
1:
	/* reload "stale" selectors */
	movl	$KDSEL,%eax
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%gs
	movl	%eax,%ss
	movl	$KPSEL,%eax
	movl	%eax,%fs

	/* reload code selector by turning return into intersegmental return */
	movl	(%esp),%eax
	pushl	%eax
	movl	$KCSEL,4(%esp)
	lret
END(lgdt)

/*
 * sys_assembly.S - System functions written in assembler
 */

#include <asm.h>

ENTRY(setESP)
	movl 4(%esp), %esp
	popl %ebp
	ret


ENTRY(returnEBP)
	movl %ebp, %eax
	ret

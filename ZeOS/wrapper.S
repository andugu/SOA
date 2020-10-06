/*
 * wrapper - Fast system calls
 */

#include <asm.h>

/*	0(ebp)  - esp anterior
	4(ebp)  - @ ret
	8(ebp)  - param1
	12(ebp) - param2
	16(ebp) - param3
	...
*/
ENTRY(write)
	pushl %ebp
	movl %esp, %ebp

	pushl %ebx					// Save context
	pushl %ecx
	pushl %edx

	movl 8(%ebp), %ebx 			// Parameter passing
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx
	movl $4, %eax				// Syscall id
	pushl %ecx					// Save ecx, edx in user stack
	pushl %esi
	lea write_return_address, %esi
	pushl %esi 	// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp
	sysenter 					// Enter the system
write_return_address:
	popl %ebp					// Restore context
	addl $4, %esp
	popl %esi
	popl %ecx
	popl %edx
	popl %ecx
	popl %ebx
	
	cmpl $0, %eax				// If result is -, move to errno
	jge write_end
	//imull $-1 // COMPTE edx es modifica
	movl %eax, errno
	movl $-1, %eax
write_end:
	movl %ebp, %esp
	popl %ebp
	ret

ENTRY(gettime)
	pushl %ebp
	movl %esp, %ebp
	movl $10, %eax				// Syscall id
	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx
	lea gettime_return_address, %edx
	pushl %edx// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp
	sysenter 					// Enter the system

gettime_return_address:
	popl %ebp					// Restore context
	addl $4, %esp
	popl %edx
	popl %ecx
	cmpl $0, %eax				// If result is -, move to errno
	jge gettime_end
	//imull $-1 // COMPTE edx es modifica
	movl %eax, errno
	movl $-1, %eax
gettime_end:
	movl %ebp, %esp
	popl %ebp
	ret

/*
 * wrapper - Fast system calls
 */

#include <asm.h>
#include <segment.h>
#include <libc.h>

/*	0(ebp)  - esp anterior
	4(ebp)  - @ ret
	8(ebp)  - param1
	12(ebp) - param2
	16(ebp) - param3
	...
*/
ENTRY(write)
	pushl %ebx					// Save context
	pushl %ecx
	pushl %edx

	movl 8(%ebp), %ebx 			// Parameter passing
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx
	movl $4, %eax				// Syscall id
	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx
	pushl write_return_address 	// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp
	sysenter 					// Enter the system
write_return_address:
	popl %ebp					// Restore context
	popl
	popl %edx
	popl %ecx
	popl %edx
	popl %ecx
	popl %ebx
	cmpl %eax, $0				// If result is -, move to errno
	jge write_end
	movl -%eax, errno
	movl $-1, %eax
write_end:
	ret

ENTRY(gettime)
	movl $10, %eax				// Syscall id
	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx
	pushl gettime_return_address// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp
	sysenter 					// Enter the system
gettime_return_address:
	popl %ebp					// Restore context
	popl
	popl %edx
	popl %ecx
	cmpl %eax, $0				// If result is -, move to errno
	jge gettime_end
	movl -%eax, errno
	movl $-1, %eax
gettime_end:
	ret
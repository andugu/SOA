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
	pushl %ebp 					// Save context
	movl %esp, %ebp
	pushl %ebx					
	pushl %ecx
	pushl %edx

	movl 8(%ebp), %ebx 			// Parameter passing
	movl 12(%ebp), %ecx
	movl 16(%ebp), %edx

	movl $4, %eax				// Syscall id

	pushl %ecx					// Save ecx, edx, esi in user stack
	pushl %edx
	pushl %esi

	lea write_return_address, %esi

	pushl %esi 					// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp

	sysenter 					// Enter the system

write_return_address:
	popl %ebp					// Restore context
	addl $4, %esp				// Remove return address
	popl %esi
	popl %edx
	popl %ecx
	popl %edx
	popl %ecx
	popl %ebx
	
	cmpl $0, %eax				// If result is -, move to errno
	jge write_end

	pushl %edx
	movl $-1, %edx
	imull %edx 					// %eax * -1 (%edx is also modified)
	popl %edx
	movl %eax, errno 			// Move error code to errno
	movl $-1, %eax				// Set return value to -1

write_end:
	movl %ebp, %esp
	popl %ebp
	ret

ENTRY(gettime)
	pushl %ebp					// Save context
	movl %esp, %ebp

	movl $10, %eax				// Syscall id

	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx

	lea gettime_return_address, %edx

	pushl %edx					// Store return address
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

	pushl %edx
	movl $-1, %edx
	imull %edx 					// %eax * -1 (%edx is also modified)
	popl %edx
	movl %eax, errno 			// Move error code to errno
	movl $-1, %eax 				// Set return value to -1

gettime_end:
	movl %ebp, %esp
	popl %ebp
	ret

ENTRY(getpid)
	pushl %ebp					// Save context
	movl %esp, %ebp

	movl $20, %eax				// Syscall id

	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx

	lea getpid_return_address, %edx

	pushl %edx					// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp

	sysenter 					// Enter the system

getpid_return_address:
	popl %ebp					// Restore context
	addl $4, %esp
	popl %edx
	popl %ecx

	cmpl $0, %eax				// If result is -, move to errno
	jge getpid_end

	pushl %edx
	movl $-1, %edx
	imull %edx 					// %eax * -1 (%edx is also modified)
	popl %edx
	movl %eax, errno 			// Move error code to errno
	movl $-1, %eax 				// Set return value to -1

getpid_end:
	movl %ebp, %esp
	popl %ebp
	ret

ENTRY(fork)
	pushl %ebp					// Save context
	movl %esp, %ebp

	movl $2, %eax				// Syscall id

	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx

	lea fork_return_address, %edx

	pushl %edx					// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp

	sysenter 					// Enter the system

fork_return_address:
	popl %ebp					// Restore context
	addl $4, %esp
	popl %edx
	popl %ecx

	cmpl $0, %eax				// If result is -, move to errno
	jge fork_end

	pushl %edx
	movl $-1, %edx
	imull %edx 					// %eax * -1 (%edx is also modified)
	popl %edx
	movl %eax, errno 			// Move error code to errno
	movl $-1, %eax 				// Set return value to -1

fork_end:
	movl %ebp, %esp
	popl %ebp
	ret

ENTRY(exit)
	pushl %ebp					// Save context
	movl %esp, %ebp

	movl $1, %eax				// Syscall id

	pushl %ecx					// Save ecx, edx in user stack
	pushl %edx

	lea exit_return_address, %edx

	pushl %edx					// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp

	sysenter 					// Enter the system

exit_return_address:
	popl %ebp					// Restore context
	addl $4, %esp
	popl %edx
	popl %ecx

	cmpl $0, %eax				// If result is -, move to errno
	jge exit_end

	pushl %edx
	movl $-1, %edx
	imull %edx 					// %eax * -1 (%edx is also modified)
	popl %edx
	movl %eax, errno 			// Move error code to errno
	movl $-1, %eax 				// Set return value to -1

exit_end:
	movl %ebp, %esp
	popl %ebp
	ret

ENTRY(getstats)
	pushl %ebp 					// Save context
	movl %esp, %ebp
	pushl %ebx					
	pushl %ecx
	pushl %edx

	movl 8(%ebp), %ebx 			// Parameter passing
	movl 12(%ebp), %ecx

	movl $35, %eax				// Syscall id

	pushl %ecx					// Save ecx, edx, esi in user stack
	pushl %edx
	pushl %esi

	lea getstats_return_address, %esi

	pushl %esi 					// Store return address
	pushl %ebp					// Create fake dynamic link
	movl %esp, %ebp

	sysenter 					// Enter the system

getstats_return_address:
	popl %ebp					// Restore context
	addl $4, %esp				// Remove return address
	popl %esi
	popl %edx
	popl %ecx
	popl %edx
	popl %ecx
	popl %ebx
	
	cmpl $0, %eax				// If result is -, move to errno
	jge getstats_end

	pushl %edx
	movl $-1, %edx
	imull %edx 					// %eax * -1 (%edx is also modified)
	popl %edx
	movl %eax, errno 			// Move error code to errno
	movl $-1, %eax				// Set return value to -1

getstats_end:
	movl %ebp, %esp
	popl %ebp
	ret

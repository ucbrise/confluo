/*
 * libhugetlbfs - direct system call for s390 (31 bit)
 *
 * Copyright IBM Corp. 2012
 * Author(s): Gerald Schaefer <gerald.schaefer@de.ibm.com>
 *
 * NOTE: Only 4 syscall parameters supported w/o stack access, but this is
 *       sufficient for the syscalls used so far (__NR_write, __NR_kill and
 *       __NR_getpid).
 */

	.text

	.globl	direct_syscall
direct_syscall:
	lr	%r1,%r2
	lr	%r2,%r3
	lr	%r3,%r4
	lr	%r4,%r5
	lr	%r5,%r6
	svc	0
	br	%r14

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

/*
 * libhugetlbfs - direct system call for s390 (64 bit)
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
	lgr	%r1,%r2
	lgr	%r2,%r3
	lgr	%r3,%r4
	lgr	%r4,%r5
	lgr	%r5,%r6
	svc	0
	br	%r14

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

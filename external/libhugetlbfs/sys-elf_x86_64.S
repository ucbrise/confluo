/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2007 David Gibson, IBM Corporation.
 *
 * Based on code from the GNU C Library, Copyright Free Software Foundation, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

	.text

	.globl	direct_syscall
direct_syscall:
	mov	%rdi,%rax
	mov	%rsi,%rdi
	mov	%rdx,%rsi
	mov	%rcx,%rdx
	mov	%r8,%r10
	mov	%r9,%r8
	mov	0x8(%rsp),%r9
	syscall
	retq

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

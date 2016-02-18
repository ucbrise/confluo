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
	push	%ebp
	push	%edi
	push	%esi
	push	%ebx
	mov	0x2c(%esp),%ebp
	mov	0x28(%esp),%edi
	mov	0x24(%esp),%esi
	mov	0x20(%esp),%edx
	mov	0x1c(%esp),%ecx
	mov	0x18(%esp),%ebx
	mov	0x14(%esp),%eax
	int	$0x80
	pop	%ebx
	pop	%esi
	pop	%edi
	pop	%ebp
	ret

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2012 ARM Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

        .globl  direct_syscall
	.type	direct_syscall,%function
direct_syscall:
	mov     ip, sp
	stmfd   sp!, {r4, r5, r6, r7}
	mov     r7, r0
	mov     r0, r1
	mov     r1, r2
	mov     r2, r3
	ldmfd   ip, {r3, r4, r5, r6}
	swi     0x0
	ldmfd   sp!, {r4, r5, r6, r7}
	bx	lr

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

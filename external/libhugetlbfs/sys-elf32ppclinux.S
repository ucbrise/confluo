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
	mr	0,3
	mr	3,4
	mr	4,5
	mr	5,6
	mr	6,7
	mr	7,8
	mr	8,9
	sc
	blr

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

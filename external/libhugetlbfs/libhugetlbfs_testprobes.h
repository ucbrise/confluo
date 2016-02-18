/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 IBM Corporation, author: Andy Whitcroft
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

/*
 * This file should only contain definitions of functions, data types, and
 * constants which are part of the internal library test probe interfaces.
 * These are exposed only to utilities and tests within the source, this is
 * not a public interface nor part of the libhugetlfs API.
 *
 * All functions declared external here must be externalised using a define
 * of the following form:
 *
 * 	#define foo __tp_foo
 */

#ifndef _LIBHUGETLBFS_TESTPROBES_H
#define _LIBHUGETLBFS_TESTPROBES_H

#define kernel_default_hugepage_size_reset \
		__tp_kernel_default_hugepage_size_reset
void kernel_default_hugepage_size_reset(void);

#endif /* _LIBHUGETLBFS_TESTPROBES_H */

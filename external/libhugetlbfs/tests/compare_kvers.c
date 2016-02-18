/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 Adam Litke, IBM Corporation.
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

#include <stdio.h>
#include <errno.h>
#include "libhugetlbfs_privutils.h"

int main (int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s <version-str> <version-str>\n", argv[0]);
		return -1;
	}

	switch (test_compare_kver(argv[1], argv[2])) {
		case  0: /* Equal to */
			return 0;
		case -1: /* Less than */
			return 1;
		case  1: /* Greater than */
			return 2;
		default:
			return -1;
	}
}

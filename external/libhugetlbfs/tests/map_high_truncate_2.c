/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2005-2006 David Gibson & Adam Litke, IBM Corporation.
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
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/*
 * Test rationale:
 *
 * At one stage, a misconversion of hugetlb_vmtruncate_list to a
 * prio_tree meant that on 32-bit machines, certain combinations of
 * mapping and truncations could truncate incorrect pages, or
 * overwrite pmds from other VMAs, triggering BUG_ON()s or other
 * wierdness.
 *
 * Test adapted to the libhugetlbfs framework from an example by
 * Kenneth Chen <kenneth.w.chen@intel.com>
 *
 * WARNING: The offsets and addresses used within are specifically
 * calculated to trigger the bug as it existed.  Don't mess with them
 * unless you *really* know what you're doing.
 *
 * The kernel bug in question was fixed with commit
 * 856fc29505556cf263f3dcda2533cf3766c14ab6.
 */
#define MAP_LENGTH	(4 * hpage_size)
#if defined(__s390__) && __WORDSIZE == 32
#define TRUNCATE_POINT 0x20000000UL
#else
#define TRUNCATE_POINT 0x60000000UL
#endif
#define HIGH_ADDR	0xa0000000UL

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	char *p, *q;
	unsigned long i;
	int err;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	check_free_huge_pages(4);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	/* First mapping */
	p = mmap(0, MAP_LENGTH + TRUNCATE_POINT, PROT_READ | PROT_WRITE,
		 MAP_PRIVATE | MAP_NORESERVE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() 1: %s", strerror(errno));

	munmap(p, 4*hpage_size + TRUNCATE_POINT);

	q = mmap((void *)HIGH_ADDR, MAP_LENGTH, PROT_READ | PROT_WRITE,
		 MAP_PRIVATE, fd, 0);
	if (q == MAP_FAILED)
		FAIL("mmap() 2: %s", strerror(errno));

	verbose_printf("High map at %p\n", q);

	for (i = 0; i < MAP_LENGTH; i += hpage_size)
		q[i] = 1;

	err = ftruncate(fd, TRUNCATE_POINT);
	if (err != 0)
		FAIL("ftruncate(): %s", strerror(errno));

	if (q[0] != 1)
		FAIL("data mismatch");

	PASS();
}

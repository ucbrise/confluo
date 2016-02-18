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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/*
 * Test rationale:
 *
 * Designed to pick up a bug on ppc64 where
 * touches_hugepage_high_range() falsely reported true for ranges
 * reaching below 4GB
 *
 * WARNING: The offsets and addresses used within are specifically
 * calculated to trigger the bug as it existed.  Don't mess with them
 * unless you *really* know what you're doing.
 */

int main(int argc, char *argv[])
{
	int page_size;
	long hpage_size;
	int fd;
	void *p, *q;
	unsigned long lowaddr;
	int err;

	test_init(argc, argv);

	page_size = getpagesize();

	hpage_size = check_hugepagesize();

	if (sizeof(void *) <= 4)
		IRRELEVANT();

	if (hpage_size > FOURGB)
		CONFIG("Huge page size is too large");

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap((void *)FOURGB, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED | MAP_FIXED, fd, 0);
	if (p == MAP_FAILED) {
		/* slice 0 (high) spans from 4G-1T */
		unsigned long below_start = FOURGB;
		unsigned long above_end = 1024L*1024*1024*1024;
		if (range_is_mapped(below_start, above_end) == 1) {
			verbose_printf("region 4G-1T is not free\n");
			verbose_printf("mmap() failed: %s\n", strerror(errno));
			PASS_INCONCLUSIVE();
		} else
			FAIL("mmap() huge: %s\n", strerror(errno));
	}
	if (p != (void *)FOURGB)
		FAIL("Wrong address with MAP_FIXED huge");

	verbose_printf("Mapped hugetlb at %p\n", p);

	memset(p, 0, hpage_size);

	err = test_addr_huge(p);
	if (err != 1)
		FAIL("Mapped address is not hugepage");

	/* Test just below 4GB to check for off-by-one errors */
	lowaddr = FOURGB - page_size;
	q = mmap((void *)lowaddr, page_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED|MAP_FIXED|MAP_ANONYMOUS, 0, 0);
	if (p == MAP_FAILED) {
		unsigned long below_start = FOURGB - page_size;
		unsigned long above_end = FOURGB;
		if (range_is_mapped(below_start, above_end) == 1) {
			verbose_printf("region (4G-page)-4G is not free\n");
			verbose_printf("mmap() failed: %s\n", strerror(errno));
			PASS_INCONCLUSIVE();
		} else
			FAIL("mmap() normal: %s\n", strerror(errno));
	}
	if (q != (void *)lowaddr)
		FAIL("Wrong address with MAP_FIXED normal");

	memset(q, 0, page_size);

	PASS();
}

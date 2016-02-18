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
 * touches_hugepage_low_range() could give false positives because of
 * the peculiar (undefined) behaviour of << for large shifts
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
	unsigned long lowaddr, highaddr;
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



	/* We use a low address right below 4GB so we can test for
	 * off-by-one errors */
	lowaddr = FOURGB - hpage_size;
	verbose_printf("Mapping hugepage at at %lx...", lowaddr);
	p = mmap((void *)lowaddr, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED|MAP_FIXED, fd, 0);
	if (p == MAP_FAILED) {
		/* This is last low slice - 256M just before 4G */
		unsigned long below_start = FOURGB - 256L*1024*1024;
		unsigned long above_end = FOURGB;
		if (range_is_mapped(below_start, above_end) == 1) {
			verbose_printf("region (4G-256M)-4G is not free\n");
			verbose_printf("mmap() failed: %s\n", strerror(errno));
			PASS_INCONCLUSIVE();
		} else
			FAIL("mmap() huge: %s\n", strerror(errno));
	}
	if (p != (void *)lowaddr)
		FAIL("Wrong address with MAP_FIXED huge");
	verbose_printf("done\n");

	memset(p, 0, hpage_size);

	err = test_addr_huge(p);
	if (err != 1)
		FAIL("Mapped address is not hugepage");

	/* Test for off by one errors */
	highaddr = FOURGB;
	verbose_printf("Mapping normal page at %lx...", highaddr);
	q = mmap((void *)highaddr, page_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED|MAP_FIXED|MAP_ANONYMOUS, 0, 0);
	if (p == MAP_FAILED) {
		unsigned long below_start = FOURGB;
		unsigned long above_end = FOURGB + page_size;
		if (range_is_mapped(below_start, above_end) == 1) {
			verbose_printf("region 4G-(4G+page) is not free\n");
			verbose_printf("mmap() failed: %s\n", strerror(errno));
			PASS_INCONCLUSIVE();
		} else
			FAIL("mmap() normal 1: %s\n", strerror(errno));
	}
	if (q != (void *)highaddr)
		FAIL("Wrong address with MAP_FIXED normal 2");
	verbose_printf("done\n");

	memset(q, 0, page_size);

	/* Why this address?  Well on ppc64, we're working with 256MB
	 * segment numbers, hence >>28.  In practice the shift
	 * instructions only start wrapping around with shifts 128 or
	 * greater. */
	highaddr = ((lowaddr >> 28) + 128) << 28;
	verbose_printf("Mapping normal page at %lx...", highaddr);
	q = mmap((void *)highaddr, page_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED|MAP_FIXED|MAP_ANONYMOUS, 0, 0);
	if (p == MAP_FAILED) {
		unsigned long below_start = highaddr;
		unsigned long above_end = highaddr + page_size;
		if (range_is_mapped(below_start, above_end) == 1) {
			verbose_printf("region haddr-(haddr+page) not free\n");
			verbose_printf("mmap() failed: %s\n", strerror(errno));
			PASS_INCONCLUSIVE();
		} else
			FAIL("mmap() normal 2: %s\n", strerror(errno));
	}
	if (q != (void *)highaddr)
		FAIL("Wrong address with MAP_FIXED normal 2");
	verbose_printf("done\n");

	memset(q, 0, page_size);

	PASS();
}

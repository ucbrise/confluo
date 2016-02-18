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

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p;
	unsigned long straddle_addr;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	if (sizeof(void *) <= 4)
		TEST_BUG("64-bit only");

	if (hpage_size > FOURGB)
		CONFIG("Huge page size too large");

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	straddle_addr = FOURGB - hpage_size;

	/* We first try to get the mapping without MAP_FIXED */
	verbose_printf("Mapping without MAP_FIXED at %lx...", straddle_addr);
	p = mmap((void *)straddle_addr, 2*hpage_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED, fd, 0);
	if (p == (void *)straddle_addr) {
		/* These tests irrelevant if we didn't get the
		 * straddle address */
		verbose_printf("done\n");

		if (test_addr_huge(p) != 1)
			FAIL("Mapped address is not hugepage");

		if (test_addr_huge(p + hpage_size) != 1)
			FAIL("Mapped address is not hugepage");

		verbose_printf("Clearing below 4GB...");
		memset(p, 0, hpage_size);
		verbose_printf("done\n");

		verbose_printf("Clearing above 4GB...");
		memset(p + hpage_size, 0, hpage_size);
		verbose_printf("done\n");
	} else {
		verbose_printf("got %p instead, never mind\n", p);
		munmap(p, 2*hpage_size);
	}

	verbose_printf("Mapping with MAP_FIXED at %lx...", straddle_addr);
	p = mmap((void *)straddle_addr, 2*hpage_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED|MAP_FIXED, fd, 0);
	if (p == MAP_FAILED) {
		/* this area crosses last low slice and first high slice */
		unsigned long below_start = FOURGB - 256L*1024*1024;
		unsigned long above_end = 1024L*1024*1024*1024;
		if (range_is_mapped(below_start, above_end) == 1) {
			verbose_printf("region (4G-256M)-1T is not free\n");
			verbose_printf("mmap() failed: %s\n", strerror(errno));
			PASS_INCONCLUSIVE();
		} else
			FAIL("mmap() FIXED failed: %s\n", strerror(errno));
	}
	if (p != (void *)straddle_addr) {
		verbose_printf("got %p instead\n", p);
		FAIL("Wrong address with MAP_FIXED");
	}
	verbose_printf("done\n");

	if (test_addr_huge(p) != 1)
		FAIL("Mapped address is not hugepage");

	if (test_addr_huge(p + hpage_size) != 1)
		FAIL("Mapped address is not hugepage");

	verbose_printf("Clearing below 4GB...");
	memset(p, 0, hpage_size);
	verbose_printf("done\n");

	verbose_printf("Clearing above 4GB...");
	memset(p + hpage_size, 0, hpage_size);
	verbose_printf("done\n");

	verbose_printf("Tested above 4GB\n");

	PASS();
}

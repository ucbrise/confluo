/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2005-2006 IBM Corporation.
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
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hugetlbfs.h>
#include "hugetests.h"

/*
 * Test rationale:
 *
 * madvise() on some kernels can cause the reservation counter to get
 * corrupted. The problem is that the patches are allocated for the
 * reservation but not faulted in at the time of allocation. The
 * counters do not get updated and effectively "leak". This test
 * identifies whether the kernel is vunerable to the problem or not.
 * It is fixed in kernel by commit f2deae9d4e70793568ef9e85d227abb7bef5b622
 */
int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p;
	unsigned long initial_rsvd, map_rsvd, madvise_rsvd, end_rsvd;

	test_init(argc, argv);

	/* Setup */
	hpage_size = check_hugepagesize();
	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");
	initial_rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count before map: %lu\n", initial_rsvd);

	/* mmap a region and record reservations */
	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	map_rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count after map: %lu\n", map_rsvd);

	/* madvise the region and record reservations */
	if (madvise(p, hpage_size, MADV_WILLNEED) == -1)
		FAIL("madvise(): %s", strerror(errno));
	madvise_rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count after madvise: %lu\n", madvise_rsvd);

	/* Free region */
	munmap(p, hpage_size);
	close(fd);
	end_rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count after close(): %lu\n", end_rsvd);

	/* Reserve count should match initial reserve count */
	if (end_rsvd != initial_rsvd)
		FAIL("Reserve leaked: %lu != %lu\n", end_rsvd, initial_rsvd);

	PASS();
}

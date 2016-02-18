/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2013 Joonsoo Kim, LG Electronics.
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
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <hugetlbfs.h>
#include "hugetests.h"

#define P "noresv-regarded-as-resv"
#define DESC \
	"* Test to correct handling for reserve count. If no reserved	   *\n"\
	"* mapping is created to reserved file region, it should be	   *\n"\
	"* considered as reserve mapping. Otherwise, reserve count will be *\n"\
	"* overflowed.							   *\n"

int main(int argc, char *argv[])
{
	long hpage_size;
	int nr_resvpages1, nr_resvpages2;
	int fd;
	char *p, *q;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	nr_resvpages1 = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Number of reserve page is %d\n", nr_resvpages1);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap(NULL, hpage_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() 1: %s", strerror(errno));

	verbose_printf("Reserve a page to file offset 0\n");

	q = mmap(NULL, hpage_size,
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd, 0);
	if (q == MAP_FAILED)
		FAIL("mmap() 2: %s", strerror(errno));

	verbose_printf("Map a page of file offset 0 with no resv mapping\n");
	*q = 's';

	munmap(p, hpage_size);
	munmap(q, hpage_size);
	close(fd);
	verbose_printf("Unmap all mappings and close file\n");

	nr_resvpages2 = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Number of reserve page is now %d\n", nr_resvpages2);

	if (nr_resvpages1 != nr_resvpages2)
		FAIL("Reserve count overflowed");

	PASS();
}

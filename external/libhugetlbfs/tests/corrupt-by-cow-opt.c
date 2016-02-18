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
#include <sys/mman.h>
#include <hugetlbfs.h>
#include "hugetests.h"

#define P "corrupt-by-cow-opt"
#define DESC \
	"* Test sanity of cow optimization on page cache. If a page	   *\n"\
	"* in page cache has only 1 ref count, it is mapped for a private  *\n"\
	"* mapping directly and is overwritten freely, so next time we	   *\n"\
	"* access the page, we can see corrupt data.			   *\n"\

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	char *p;
	char c;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	check_free_huge_pages(2);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap(NULL, hpage_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() 1: %s", strerror(errno));

	*p = 's';
	verbose_printf("Write %c to %p via shared mapping\n", *p, p);
	munmap(p, hpage_size);

	p = mmap(NULL, hpage_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() 2: %s", strerror(errno));

	*p = 'p';
	verbose_printf("Write %c to %p via private mapping\n", *p, p);
	munmap(p, hpage_size);

	p = mmap(NULL, hpage_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() 3: %s", strerror(errno));

	c = *p;
	verbose_printf("Read %c from %p via shared mapping\n", *p, p);
	munmap(p, hpage_size);

	if (c != 's')
		FAIL("data corrupt");

	PASS();
}

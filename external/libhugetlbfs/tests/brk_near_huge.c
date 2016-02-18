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
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/*
 * Test rationale:
 *
 * Certain kernels have a bug where brk() does not perform the same
 * checks that a MAP_FIXED mmap() will, allowing brk() to create a
 * normal page VMA in a hugepage only address region.  This can lead
 * to oopses or other badness.
 */

/* Possibly these functions should go in the library itself.. */
#ifdef __powerpc64__
void *next_chunk(void *addr)
{
	if ((unsigned long)addr < 0x100000000UL)
		/* 256M segments below 4G */
		return PALIGN(addr, 0x10000000UL);
	else
		/* 1TB segments above */
		return PALIGN(addr, 0x10000000000UL);
}
#elif defined(__powerpc__) && !defined(PPC_NO_SEGMENTS)
void *next_chunk(void *addr)
{
	return PALIGN(addr, 0x10000000UL);
}
#elif defined(__ia64__)
void *next_chunk(void *addr)
{
	return PALIGN(addr, 0x8000000000000000UL);
}
#else
void *next_chunk(void *addr)
{
	return PALIGN(addr, gethugepagesize());
}
#endif

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *brk0, *hugemap_addr, *newbrk;
	char *p;
	int err;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	brk0 = sbrk(0);
	verbose_printf("Initial break at %p\n", brk0);

	hugemap_addr = next_chunk(brk0) + hpage_size;

	p = mmap(hugemap_addr, hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	if (p != hugemap_addr)
		FAIL("mmap() at unexpected address %p instead of %p\n", p,
		     hugemap_addr);

	verbose_printf("Hugepage mapped at %p-%p\n", p, p+hpage_size-1);

	err = test_addr_huge((void *)p);
	if (err != 1)
		FAIL("Mapped address is not hugepage");

	newbrk = next_chunk(brk0) + getpagesize();
	err = brk((void *)newbrk);
	if (err == -1)
		/* Failing the brk() is an acceptable kernel response */
		PASS();

	/* Suceeding the brk() is acceptable iff the new memory is
	 * properly accesible and we don't have a kernel blow up when
	 * we touch it. */
	memset(brk0, 0, newbrk-brk0);

	PASS();
}

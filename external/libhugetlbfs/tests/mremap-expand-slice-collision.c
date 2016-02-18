/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2009 David Gibson, IBM Corporation.
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define RANDOM_CONSTANT	0x1234ABCD

unsigned long slice_boundary;
long hpage_size, page_size;

void init_slice_boundary(int fd)
{
	unsigned long slice_size;
	void *p, *heap;
	int i, rc;
#if defined(__LP64__) && !defined(__aarch64__)
	/* powerpc: 1TB slices starting at 1 TB */
	slice_boundary = 0x10000000000;
	slice_size = 0x10000000000;
#else
	/* powerpc: 256MB slices up to 4GB */
	slice_boundary = 0x00000000;
	slice_size = 0x10000000;
#endif

	/* dummy malloc so we know where is heap */
	heap = malloc(1);
	free(heap);

	/* Find 2 neighbour slices with couple huge pages free
	 * around slice boundary.
	 * 16 is the maximum number of slices (low/high) */
	for (i = 0; i < 16-1; i++) {
		slice_boundary += slice_size;
		p = mmap((void *)(slice_boundary-2*hpage_size), 4*hpage_size,
			PROT_READ, MAP_SHARED | MAP_FIXED, fd, 0);
		if (p == MAP_FAILED) {
			verbose_printf("can't use slice_boundary: 0x%lx\n",
				slice_boundary);
		} else {
			rc = munmap(p, 4*hpage_size);
			if (rc != 0)
				FAIL("munmap(p1): %s", strerror(errno));
			break;
		}
	}

	if (p == MAP_FAILED)
		FAIL("couldn't find 2 free neighbour slices");
	verbose_printf("using slice_boundary: 0x%lx\n", slice_boundary);
}

void do_readback(void *p, size_t size, const char *stage)
{
	unsigned int *q = p;
	int i;

	verbose_printf("do_readback(%p, 0x%lx, \"%s\")\n", p,
		       (unsigned long)size, stage);

	for (i = 0; i < (size / sizeof(*q)); i++) {
		q[i] = RANDOM_CONSTANT ^ i;
	}

	for (i = 0; i < (size / sizeof(*q)); i++) {
		if (q[i] != (RANDOM_CONSTANT ^ i))
			FAIL("Stage \"%s\": Mismatch at offset 0x%x: 0x%x instead of 0x%x",
			     stage, i, q[i], RANDOM_CONSTANT ^ i);
	}
}

void do_remap(int fd, void *target)
{
	void *a, *b;
	int rc;

	a = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (a == MAP_FAILED)
		FAIL("mmap(huge page): %s", strerror(errno));

	verbose_printf("Huge base mapping at %p\n", a);

	do_readback(a, hpage_size, "base huge");

	verbose_printf("Attempting mremap(MAYMOVE|FIXED) to %p...", target);

	b = mremap(a, hpage_size, hpage_size, MREMAP_MAYMOVE | MREMAP_FIXED,
		   target);

	if (b != MAP_FAILED) {
		verbose_printf("testing...");
		do_readback(b, hpage_size, "remapped");
		verbose_printf("ok\n");
	} else {
		verbose_printf("disallowed (%s)\n", strerror(errno));
	}

	rc = munmap(b, hpage_size);
	if (rc != 0)
		FAIL("munmap(after remap): %s", strerror(errno));
}

int main(int argc, char *argv[])
{
	int fd, rc;
	void *p, *q, *r;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	page_size = getpagesize();


	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");
	init_slice_boundary(fd);

	/* First, hugepages above, normal below */
	p = mmap((void *)(slice_boundary + hpage_size), hpage_size,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED | MAP_FIXED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(huge above): %s", strerror(errno));

	do_readback(p, hpage_size, "huge above");

	q = mmap((void *)(slice_boundary - page_size), page_size,
		 PROT_READ | PROT_WRITE,
		 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	if (q == MAP_FAILED)
		FAIL("mmap(normal below): %s", strerror(errno));

	do_readback(q, page_size, "normal below");

	verbose_printf("Attempting to remap...");

	r = mremap(q, page_size, 2*page_size, 0);
	if (r == MAP_FAILED) {
		verbose_printf("disallowed\n");
		rc = munmap(q, page_size);
		if (rc != 0)
			FAIL("munmap(normal below): %s", strerror(errno));
	} else {
		if (r != q)
			FAIL("mremap() moved without MREMAP_MAYMOVE!?");

		verbose_printf("testing...");
		do_readback(q, 2*page_size, "normal below expanded");
		rc = munmap(q, 2*page_size);
		if (rc != 0)
			FAIL("munmap(normal below expanded): %s", strerror(errno));
	}

	rc = munmap(p, hpage_size);
	if (rc != 0)
		FAIL("munmap(huge above)");

	/* Next, normal pages above, huge below */
	p = mmap((void *)(slice_boundary + hpage_size), page_size,
		 PROT_READ|PROT_WRITE,
		 MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(normal above): %s", strerror(errno));

	do_readback(p, page_size, "normal above");

	q = mmap((void *)(slice_boundary - hpage_size),
		 hpage_size, PROT_READ | PROT_WRITE,
		 MAP_SHARED | MAP_FIXED, fd, 0);
	if (q == MAP_FAILED)
		FAIL("mmap(huge below): %s", strerror(errno));

	do_readback(q, hpage_size, "huge below");

	verbose_printf("Attempting to remap...");

	r = mremap(q, hpage_size, 2*hpage_size, 0);
	if (r == MAP_FAILED) {
		verbose_printf("disallowed\n");
		rc = munmap(q, hpage_size);
		if (rc != 0)
			FAIL("munmap(huge below): %s", strerror(errno));
	} else {
		if (r != q)
			FAIL("mremap() moved without MREMAP_MAYMOVE!?");

		verbose_printf("testing...");
		do_readback(q, 2*hpage_size, "huge below expanded");
		rc = munmap(q, 2*hpage_size);
		if (rc != 0)
			FAIL("munmap(huge below expanded): %s", strerror(errno));
	}

	rc = munmap(p, page_size);
	if (rc != 0)
		FAIL("munmap(normal above)");


	PASS();
}

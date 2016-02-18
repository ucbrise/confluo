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

long page_size, hpage_size;

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

void do_remap(void *target)
{
	void *a, *b;
	int rc;

	a = mmap(NULL, page_size, PROT_READ|PROT_WRITE,
		  MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if (a == MAP_FAILED)
		FAIL("mmap(normal page): %s", strerror(errno));

	verbose_printf("Normal base mapping at %p\n", a);

	do_readback(a, page_size, "base normal");

	verbose_printf("Attempting mremap(MAYMOVE|FIXED) to %p...", target);

	b = mremap(a, page_size, page_size, MREMAP_MAYMOVE | MREMAP_FIXED,
		   target);

	if (b != MAP_FAILED) {
		verbose_printf("testing...");
		do_readback(b, page_size, "remapped");
		verbose_printf("ok\n");
	} else {
		verbose_printf("disallowed (%s)\n", strerror(errno));
		b = a;
	}

	rc = munmap(b, page_size);
	if (rc != 0)
		FAIL("munmap(after remap): %s", strerror(errno));
}

int main(int argc, char *argv[])
{
	int fd, rc;
	void *p;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	page_size = getpagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap(NULL, 3*hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));

	rc = munmap(p, hpage_size);
	if (rc != 0)
		FAIL("munmap() low hpage: %s", strerror(errno));

	rc = munmap(p + 2*hpage_size, hpage_size);
	if (rc != 0)
		FAIL("munmap() high hpage: %s", strerror(errno));

	p = p + hpage_size;

	verbose_printf("Hugepage mapping at %p\n", p);

	do_readback(p, hpage_size, "base hugepage");

	do_remap(p - page_size);
	do_remap(p + hpage_size);

	PASS();
}

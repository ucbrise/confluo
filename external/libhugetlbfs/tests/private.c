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

#define RANDOM_CONSTANT	0x1234ABCD
#define OTHER_CONSTANT  0xFEDC9876

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p, *q;
	unsigned int *pl, *ql;
	int i;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() SHARED: %s", strerror(errno));

	pl = p;
	for (i = 0; i < (hpage_size / sizeof(*pl)); i++) {
		pl[i] = RANDOM_CONSTANT ^ i;
	}

	q = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE,
		 fd, 0);
	if (q == MAP_FAILED)
		FAIL("mmap() PRIVATE: %s", strerror(errno));

	ql = q;
	for (i = 0; i < (hpage_size / sizeof(*ql)); i++) {
		if (ql[i] != (RANDOM_CONSTANT ^ i))
			FAIL("Mismatch");
	}

	for (i = 0; i < (hpage_size / sizeof(*ql)); i++) {
		ql[i] = OTHER_CONSTANT ^ i;
	}

	for (i = 0; i < (hpage_size / sizeof(*ql)); i++) {
		if (ql[i] != (OTHER_CONSTANT ^ i))
			FAIL("PRIVATE mismatch");
	}

	for (i = 0; i < (hpage_size / sizeof(*pl)); i++) {
		if (pl[i] != (RANDOM_CONSTANT ^ i))
			FAIL("SHARED map contaminated");
	}

	memset(p, 0, hpage_size);

	for (i = 0; i < (hpage_size / sizeof(*ql)); i++) {
		if (ql[i] != (OTHER_CONSTANT ^ i))
			FAIL("PRIVATE map contaminated");
	}

	PASS();
}

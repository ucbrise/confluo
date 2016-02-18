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
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/*
 * Test rationale:
 *
 * Some kernel have a bug in the positioning of the test against
 * i_size.  This bug means that attempting to instantiate a page
 * beyond the end of a hugepage file can result in an OOM and SIGKILL
 * instead of the correct SIGBUS.
 *
 * This bug was fixed by commit ebed4bfc8da8df5b6b0bc4a5064a949f04683509.
 */
static void sigbus_handler(int signum, siginfo_t *si, void *uc)
{
	PASS();
}

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd, fdx;
	unsigned long totpages;
	void *p, *q;
	int i;
	int err;
	struct sigaction sa = {
		.sa_sigaction = sigbus_handler,
		.sa_flags = SA_SIGINFO,
	};

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	totpages = get_huge_page_counter(hpage_size, HUGEPAGES_TOTAL);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	err = ftruncate(fd, 0);
	if (err)
		FAIL("ftruncate(): %s", strerror(errno));

	/* Now slurp up all the available pages */
	fdx = hugetlbfs_unlinked_fd();
	if (fdx < 0)
		FAIL("hugetlbfs_unlinked_fd() 2");

	q = mmap(NULL, totpages * hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fdx, 0);
	if (q == MAP_FAILED)
		FAIL("mmap() reserving all pages: %s", strerror(errno));

	/* Touch the pages to ensure they're removed from the pool */
	for (i = 0; i < totpages; i++) {
		volatile char *x = (volatile char *)q + i*hpage_size;
		*x = 0;
	}

	/* SIGBUS is what *should* happen */
	err = sigaction(SIGBUS, &sa, NULL);
	if (err)
		FAIL("sigaction(): %s", strerror(errno));

	*((volatile unsigned int *)p);

	/* Should have SIGBUSed above, or (failed the test) with SIGKILL */
	FAIL("Didn't SIGBUS or OOM");
}

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
#include <signal.h>
#include <sys/mman.h>
#include <setjmp.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/*
 * Test rationale:
 *
 * At one stage, improper handling of tests against i_size could mess
 * up accounting of reserved hugepages on certain truncate
 * operations.
 *
 * This bug was fixed with a band-aid (enough to pass this test) in
 * commit ebed4bfc8da8df5b6b0bc4a5064a949f04683509.  A more complete
 * fix still pending as of 3d4248885b9fca818e7fe6b66328e714876d36ad.
 */

#define RANDOM_CONSTANT	0x1234ABCD

static sigjmp_buf sig_escape;

static void sigbus_handler(int signum, siginfo_t *si, void *uc)
{
	siglongjmp(sig_escape, 17);
}

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p;
	volatile unsigned int *q;
	int err;
	int sigbus_count = 0;
	unsigned long initial_rsvd, rsvd;
	struct sigaction sa = {
		.sa_sigaction = sigbus_handler,
		.sa_flags = SA_SIGINFO,
	};

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	initial_rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count before map: %lu\n", initial_rsvd);

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	q = p;

	verbose_printf("Reserve count after map: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_RSVD));

	*q = 0;
	verbose_printf("Reserve count after touch: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_RSVD));

	err = ftruncate(fd, 0);
	if (err)
		FAIL("ftruncate(): %s", strerror(errno));

	rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count after truncate: %lu\n", rsvd);
	if (rsvd != initial_rsvd)
		FAIL("Reserved count is not restored after truncate: %lu instead of %lu",
		     rsvd, initial_rsvd);

	err = sigaction(SIGBUS, &sa, NULL);
	if (err)
		FAIL("sigaction(): %s", strerror(errno));

	if (sigsetjmp(sig_escape, 1) == 0)
		*q; /* Fault, triggering a SIGBUS */
	else
		sigbus_count++;

	if (sigbus_count != 1)
		FAIL("Didn't SIGBUS after truncate");

	rsvd = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	verbose_printf("Reserve count after SIGBUS fault: %lu\n", rsvd);
	if (rsvd != initial_rsvd)
		FAIL("Reserved count is altered by SIGBUS fault: %lu instead of %lu",
		     rsvd, initial_rsvd);

	munmap(p, hpage_size);

	verbose_printf("Reserve count after munmap(): %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_RSVD));

	close(fd);

	verbose_printf("Reserve count after close(): %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_RSVD));

	PASS();
}

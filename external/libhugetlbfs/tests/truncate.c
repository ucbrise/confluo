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
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define RANDOM_CONSTANT	0x1234ABCD

static void sigbus_handler(int signum, siginfo_t *si, void *uc)
{
	PASS();
}

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p;
	volatile unsigned int *q;
	int err;
	struct sigaction sa = {
		.sa_sigaction = sigbus_handler,
		.sa_flags = SA_SIGINFO,
	};

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));

	q = p;

	/* Touch the memory */
	*q = 0;

	err = sigaction(SIGBUS, &sa, NULL);
	if (err)
		FAIL("sigaction(): %s", strerror(errno));


	err = ftruncate(fd, 0);
	if (err)
		FAIL("ftruncate(): %s", strerror(errno));

	*q;

	/* Should have SIGBUSed above */
	FAIL("Didn't SIGBUS");
}

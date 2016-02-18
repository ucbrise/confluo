/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 20015 Mike Kravetz, Oracle Corporation
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
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/falloc.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define P "fallocate-basic"
#define DESC \
	"* Test basic fallocate functionality in hugetlbfs.  Preallocate   *\n"\
	"* huge pages to a file in hugetlbfs, and then remove the pages    *\n"\
	"* via hole punch.                                                 *"

#define min(a,b) (((a) < (b)) ? (a) : (b))

#define MAX_PAGES_TO_USE 5

int main(int argc, char *argv[])
{
	long hpage_size;
	long nr_hpages_free;
	int fd;
	int err;
	int max_iterations;
	unsigned long free_before, free_after;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	nr_hpages_free = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	max_iterations = min(nr_hpages_free, MAX_PAGES_TO_USE);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	free_before = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);

	/* First preallocate file with max_iterations pages */
	err = fallocate(fd, 0, 0, hpage_size * max_iterations);
	if (err) {
		if (errno == EOPNOTSUPP)
			IRRELEVANT();
		if (err)
			FAIL("fallocate(): %s", strerror(errno));
	}

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_before - free_after != max_iterations)
		FAIL("fallocate did not preallocate %u huge pages\n",
							max_iterations);

	/* Now punch a hole of the same size */
	err = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			0, hpage_size * max_iterations);
	if (err)
		FAIL("fallocate(FALLOC_FL_PUNCH_HOLE): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_after != free_before)
		FAIL("fallocate hole punch did not release %u huge pages\n",
							max_iterations);
	PASS();
}

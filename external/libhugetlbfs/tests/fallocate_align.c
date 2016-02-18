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

#define P "fallocate-align"
#define DESC \
	"* Test alignment of fallocate arguments.  fallocate will take     *\n"\
	"* non-huge page aligned offsets and addresses.  However,          *\n"\
	"* operations are only performed on huge pages.  This is different *\n"\
	"* that than fallocate behavior in "normal" filesystems.           *"

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	int err;
	unsigned long free_before, free_after;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	free_before = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);

	/*
	 * First preallocate file with with just 1 byte.  Allocation sizes
	 * are rounded up, so we should get an entire huge page.
	 */
	err = fallocate(fd, 0, 0, 1);
	if (err) {
		if (errno == EOPNOTSUPP)
			IRRELEVANT();
		if (err)
			FAIL("fallocate(): %s", strerror(errno));
	}

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_before - free_after != 1)
		FAIL("fallocate 1 byte did not preallocate entire huge page\n");

	/*
	 * Now punch a hole with just 1 byte.  On hole punch, sizes are
	 * rounded down.  So, this operation should not create a hole.
	 */
	err = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			0, 1);
	if (err)
		FAIL("fallocate(FALLOC_FL_PUNCH_HOLE): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_after == free_before)
		FAIL("fallocate hole punch 1 byte free'ed a huge page\n");

	/*
	 * Now punch a hole with of 2 * hpage_size - 1 byte.  This size
	 * should be rounded down to a single huge page and the hole created.
	 */
	err = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			0, (2 * hpage_size) - 1);
	if (err)
		FAIL("fallocate(FALLOC_FL_PUNCH_HOLE): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_after != free_before)
		FAIL("fallocate hole punch 2 * hpage_size - 1 byte did not free huge page\n");

	/*
	 * Perform a preallocate operation with offset 1 and size of
	 * hpage_size.  The offset should be rounded down and the
	 * size rounded up to preallocate two huge pages.
	 */
	err = fallocate(fd, 0, 1, hpage_size);
	if (err)
		FAIL("fallocate(): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_before - free_after != 2)
		FAIL("fallocate 1 byte offset, huge page size did not preallocate two huge pages\n");

	/*
	 * The hole punch code will only delete 'whole' huge pags that are
	 * in the specified range.  The offset is rounded up, and (offset
	 * + size) is rounded down to determine the huge pages to be deleted.
	 * In this case, after rounding the range is (hpage_size, hpage_size).
	 * So, no pages should be deleted.
	 */
	err = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			1, hpage_size);
	if (err)
		FAIL("fallocate(FALLOC_FL_PUNCH_HOLE): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_before - free_after != 2)
		FAIL("fallocate hole punch 1 byte offset, huge page size incorrectly deleted a huge page\n");

	/*
	 * To delete both huge pages, the range passed to hole punch must
	 * overlap the allocated pages
	 */
	err = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			0, 2 * hpage_size);
	if (err)
		FAIL("fallocate(FALLOC_FL_PUNCH_HOLE): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_after != free_before)
		FAIL("fallocate hole punch did not delete two huge pages\n");

	PASS();
}

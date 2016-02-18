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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define P0 "ffffffff"
#define IOSZ 4096
char buf[IOSZ] __attribute__ ((aligned (IOSZ)));
#define TMPFILE "/tmp/direct"

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd, dfd;
	void *p;
	size_t ret;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	dfd = open(TMPFILE, O_CREAT|O_EXCL|O_DIRECT|O_RDWR, 0600);
	if (dfd < 0)
		CONFIG("Failed to open direct-IO file: %s", strerror(errno));
	unlink(TMPFILE);

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap hugetlbfs file: %s", strerror(errno));

	memcpy(p, P0, 8);

	/* Direct write from huge page */
	ret = write(dfd, p, IOSZ);
	if (ret == -1)
		FAIL("Direct-IO write from huge page: %s", strerror(errno));
	if (ret != IOSZ)
		FAIL("Short direct-IO write from huge page");
	if (lseek(dfd, 0, SEEK_SET) == -1)
		FAIL("lseek: %s", strerror(errno));

	/* Check for accuracy */
	ret = read(dfd, buf, IOSZ);
	if (ret == -1)
		FAIL("Direct-IO read to normal memory: %s", strerror(errno));
	if (ret != IOSZ)
		FAIL("Short direct-IO read to normal memory");
	if (memcmp(P0, buf, 8))
		FAIL("Memory mismatch after Direct-IO write");
	if (lseek(dfd, 0, SEEK_SET) == -1)
		FAIL("lseek: %s", strerror(errno));

	/* Direct read to huge page */
	memset(p, 0, IOSZ);
	ret = read(dfd, p, IOSZ);
	if (ret == -1)
		FAIL("Direct-IO read to huge page: %s\n", strerror(errno));
	if (ret != IOSZ)
		FAIL("Short direct-IO read to huge page");

	/* Check for accuracy */
	if (memcmp(p, P0, 8))
		FAIL("Memory mismatch after Direct-IO read");

	close(dfd);
	unlink(TMPFILE);

	PASS();
}

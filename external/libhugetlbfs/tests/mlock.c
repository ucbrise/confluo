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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>

#include <hugetlbfs.h>
#include "hugetests.h"

static void test_simple_mlock(int flags)
{
	int fd = hugetlbfs_unlinked_fd();
	void *p;
	int ret;
	long hpage_size = check_hugepagesize();

	p = mmap(0, hpage_size, PROT_READ|PROT_WRITE, flags, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() failed (flags=%x): %s", flags, strerror(errno));

	ret = mlock(p, hpage_size);
	if (ret)
		FAIL("mlock() failed (flags=%x): %s", flags, strerror(errno));

	ret = munlock(p, hpage_size);
	if (ret)
		FAIL("munlock() failed (flags=%x): %s", flags, strerror(errno));

	ret = munmap(p, hpage_size);
	if (ret)
		FAIL("munmap() failed (flags=%x): %s", flags, strerror(errno));

	close(fd);
}

int main(int argc, char *argv[])
{
	struct rlimit limit_info;
	if(getrlimit(RLIMIT_MEMLOCK, &limit_info))
		ERROR("Unable to read locked memory rlimit: %s", strerror(errno));
	if(limit_info.rlim_cur < check_hugepagesize())
		CONFIG("Locked memory ulimit set below huge page size");

	test_simple_mlock(MAP_PRIVATE);
	test_simple_mlock(MAP_SHARED);
	test_simple_mlock(MAP_PRIVATE|MAP_LOCKED);
	test_simple_mlock(MAP_SHARED|MAP_LOCKED);
	PASS();
}

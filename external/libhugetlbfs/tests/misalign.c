/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2005-2007 David Gibson & Adam Litke, IBM Corporation.
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
#include <sys/utsname.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/*
 * Test rationale:
 *
 * Just as normal mmap()s can't have an address, length or offset
 * which is not page aligned, so hugepage mmap()s can't have an
 * address, length or offset with is not hugepage aligned.
 *
 * However, from time to time when the various mmap() /
 * get_unmapped_area() paths are updated, somebody misses one of the
 * necessary checks for the hugepage paths.  This testcase ensures
 * that attempted hugepage mappings with parameters which are not
 * correctly hugepage aligned are rejected.
 *
 * However starting with 3.10-rc1, length passed in mmap() doesn't need
 * to be aligned because commit af73e4d9506d3b797509f3c030e7dcd554f7d9c4
 * added ALIGN() to kernel side, in mmap_pgoff(), when mapping huge page
 * files.
 */
int main(int argc, char *argv[])
{
	long page_size, hpage_size;
	int fd;
	void *p, *q;
	int err;
	struct utsname buf;

	test_init(argc, argv);

	if (uname(&buf) != 0)
		FAIL("uname failed %s", strerror(errno));

	page_size = getpagesize();
	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	/* First see what an ok mapping looks like, as a basis for our
	 * bad addresses and so forth */
	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() without hint failed: %s", strerror(errno));
	if (((unsigned long)p % hpage_size) != 0)
		FAIL("mmap() without hint at misaligned address");

	verbose_printf("Mapped at %p, length 0x%lx\n", p, hpage_size);

	err = munmap(p, hpage_size);
	if (err != 0)
		FAIL("munmap() without hint failed: %s", strerror(errno));

	/* 1) Try a misaligned hint address */
	q = mmap(p + page_size, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_PRIVATE, fd, 0);
	if (q == MAP_FAILED)
		/* Bad hint shouldn't fail, just ignore the hint */
		FAIL("mmap() with hint failed: %s", strerror(errno));
	if (((unsigned long)q % hpage_size) != 0)
		FAIL("mmap() with hint at misaligned address");

	err = munmap(q, hpage_size);
	if (err != 0)
		FAIL("munmap() with hint failed: %s", strerror(errno));

	/* 2) Try a misaligned address with MAP_FIXED */
	q = mmap(p + page_size, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_PRIVATE|MAP_FIXED, fd, 0);
	if (q != MAP_FAILED)
		FAIL("mmap() MAP_FIXED at misaligned address succeeded");

	/* 3) Try a misaligned length */
	q = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);

	if (test_compare_kver(buf.release, "3.10.0") < 0) {
		if (q != MAP_FAILED)
			FAIL("mmap() with misaligned length 0x%lx succeeded",
				page_size);
	} else {
		if (q == MAP_FAILED)
			FAIL("mmap() with misaligned length 0x%lx failed",
				page_size);
	}

	/* 4) Try a misaligned length with MAP_FIXED */
	q = mmap(p, page_size, PROT_READ|PROT_WRITE,
		 MAP_PRIVATE|MAP_FIXED, fd, 0);

	if (test_compare_kver(buf.release, "3.10.0") < 0) {
		if (q != MAP_FAILED)
			FAIL("mmap() MAP_FIXED with misaligned length 0x%lx "
				"succeeded", page_size);
	} else {
		if (q == MAP_FAILED)
			FAIL("mmap() MAP_FIXED with misaligned length 0x%lx "
				"failed", page_size);
	}

	/* 5) Try a misaligned offset */
	q = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_PRIVATE, fd, page_size);
	if (q != MAP_FAILED)
		FAIL("mmap() with misaligned offset 0x%lx succeeded",
		     page_size);

	/* 6) Try a misaligned offset with MAP_FIXED*/
	q = mmap(p, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_PRIVATE|MAP_FIXED, fd, page_size);
	if (q != MAP_FAILED)
		FAIL("mmap() MAP_FIXED with misaligned offset 0x%lx succeeded",
		     page_size);

	PASS();
}

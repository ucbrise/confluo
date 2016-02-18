/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2005-2006 David Gibson & Adam Litke, IBM Corporation.
 * Copyright (C) 2006 Hugh Dickins <hugh@veritas.com>
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
 * At one stage, a misconversion of hugetlb_vmtruncate_list to a
 * prio_tree meant that on 32-bit machines, truncates at or above 4GB
 * could truncate lower pages, resulting in BUG_ON()s.
 *
 * WARNING: The offsets and addresses used within are specifically
 * calculated to trigger the bug as it existed.  Don't mess with them
 * unless you *really* know what you're doing.
 *
 * The kernel bug in question was fixed with commit
 * 856fc29505556cf263f3dcda2533cf3766c14ab6.
 */

#define RANDOM_CONSTANT	0x1234ABCD

int main(int argc, char *argv[])
{
	int page_size;
	long hpage_size;
	off_t buggy_offset;
	int fd;
	void *p, *q;
	volatile int *pi;
	int err;

	test_init(argc, argv);

	page_size = getpagesize();
	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	/* First, we make a 2 page sane hugepage mapping.  Then we
	 * memset() it to ensure that the ptes are instantiated for
	 * it.  Then we attempt to replace the second half of the map
	 * with one at a bogus offset.  We leave the first page of
	 * sane mapping in place to ensure that the corresponding
	 * pud/pmd/whatever entries aren't cleaned away.  It's those
	 * bad entries which can trigger bad_pud() checks if the
	 * backout path for the bogus mapping is buggy, which it was
	 * in some kernels. */

	verbose_printf("Free hugepages: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_FREE));

	verbose_printf("Mapping reference map...");
	/* First get arena of three hpages size, at file offset 4GB */
	p = mmap(NULL, 2*hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() offset 4GB: %s", strerror(errno));
	verbose_printf("%p-%p\n", p, p+2*hpage_size-1);

	verbose_printf("Free hugepages: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_FREE));

	/* Instantiate the pages */
	verbose_printf("Instantiating...");
	memset(p, 0, 2*hpage_size);
	pi = p;
	*pi = RANDOM_CONSTANT;
	verbose_printf("done.\n");

	verbose_printf("Free hugepages: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_FREE));

	/* Toggle the permissions on the first page.  This forces TLB
	 * entries (including hash page table on powerpc) to be
	 * flushed, so that the page tables must be accessed for the
	 * test further down.  In the buggy case, those page tables
	 * can get thrown away by a pud_clear() */
	err = mprotect(p, hpage_size, PROT_READ);
	if (err)
		FAIL("mprotect(%p, 0x%lx, PROT_READ): %s", p, hpage_size,
							strerror(errno));

	/* Replace top hpage by hpage mapping at confusing file offset */
	buggy_offset = page_size;
	verbose_printf("Replacing map at %p with map from offset 0x%lx...",
		       p + hpage_size, (unsigned long)buggy_offset);
	q = mmap(p + hpage_size, hpage_size, PROT_READ|PROT_WRITE,
		 MAP_FIXED|MAP_PRIVATE, fd, buggy_offset);
	if (q != MAP_FAILED)
		FAIL("bogus offset mmap() succeeded at %p: %s", q, strerror(errno));
	if (errno != EINVAL)
		FAIL("bogus mmap() failed with \"%s\" instead of \"%s\"",
		     strerror(errno), strerror(EINVAL));
	verbose_printf("%s\n", strerror(errno));

	verbose_printf("Free hugepages: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_FREE));

	if (*pi != RANDOM_CONSTANT)
		FAIL("Pre-existing mapping clobbered: %x instead of %x",
		     *pi, RANDOM_CONSTANT);

	verbose_printf("Free hugepages: %lu\n",
		get_huge_page_counter(hpage_size, HUGEPAGES_FREE));

	/* The real test is whether we got a bad_pud() or similar
	 * during the run.  The check above, combined with the earlier
	 * mprotect()s to flush the TLB are supposed to catch it, but
	 * it's hard to be certain.  Once bad_pud() is called
	 * behaviour can be very strange. */
	PASS_INCONCLUSIVE();
}

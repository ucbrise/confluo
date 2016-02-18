/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 David Gibson & Adam Litke, IBM Corporation.
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

#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hugetlbfs.h"
#include "libhugetlbfs_privutils.h"
#include "hugetests.h"

void check_free_huge_pages(int nr_pages_needed)
{
	long hpage_size = gethugepagesize();
	int freepages = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (freepages < nr_pages_needed)
		CONFIG("Must have at least %i free hugepages", nr_pages_needed);
}

int using_system_hpage_size(const char *mount)
{
	struct statfs64 sb;
	int err;
	long meminfo_size, mount_size;

	if (!mount)
		FAIL("using_system_hpage_size: hugetlbfs is not mounted\n");

	err = statfs64(mount, &sb);
	if (err)
		FAIL("statfs64: %s\n", strerror(errno));

	meminfo_size = read_meminfo("Hugepagesize:");
	if (meminfo_size < 0)
		FAIL("using_system_hpage_size: Failed to read /proc/meminfo\n");

	mount_size = sb.f_bsize / 1024; /* Compare to meminfo in kB */
	if (mount_size == meminfo_size)
		return 1;
	else
		return 0;
}

/* WARNING: This function relies on the hugetlb pool counters in a way that
 * is known to be racy.  Due to the expected usage of hugetlbfs test cases, the
 * risk of a race is acceptible.  This function should NOT be used for real
 * applications.
 */
int kernel_has_private_reservations(void)
{
	int fd;
	long t, f, r, s;
	long nt, nf, nr, ns;
	long hpage_size = gethugepagesize();
	void *map;

	/* Read pool counters */
	t = get_huge_page_counter(hpage_size, HUGEPAGES_TOTAL);
	f = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	r = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	s = get_huge_page_counter(hpage_size, HUGEPAGES_SURP);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0) {
		ERROR("kernel_has_private_reservations: hugetlbfs_unlinked_fd: "
			"%s\n", strerror(errno));
		return -1;
	}
	map = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		ERROR("kernel_has_private_reservations: mmap: %s\n",
			strerror(errno));
		return -1;
	}

	/* Recheck the counters */
	nt = get_huge_page_counter(hpage_size, HUGEPAGES_TOTAL);
	nf = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	nr = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	ns = get_huge_page_counter(hpage_size, HUGEPAGES_SURP);

	munmap(map, hpage_size);
	close(fd);

	/*
	 * There are only three valid cases:
	 * 1) If a surplus page was allocated to create a reservation, all
	 *    four pool counters increment
	 * 2) All counters remain the same except for Hugepages_Rsvd, then
	 *    a reservation was created using an existing pool page.
	 * 3) All counters remain the same, indicates that no reservation has
	 *    been created
	 */
	if ((nt == t + 1) && (nf == f + 1) && (ns == s + 1) && (nr == r + 1)) {
		return 1;
	} else if ((nt == t) && (nf == f) && (ns == s)) {
		if (nr == r + 1)
			return 1;
		else if (nr == r)
			return 0;
	} else {
		ERROR("kernel_has_private_reservations: bad counter state - "
		      "T:%li F:%li R:%li S:%li -> T:%li F:%li R:%li S:%li\n",
			t, f, r, s, nt, nf, nr, ns);
	}
	return -1;
}

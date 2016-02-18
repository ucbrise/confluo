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
/* Test rationale:
 *
 * ppc64 kernels (prior to 2.6.15-rc5) have a bug in the hugepage SLB
 * flushing path.  After opening new hugetlb areas, we update the
 * masks in the thread_struct, copy to the PACA, then do slbies on
 * each CPU.  The trouble is we only copy to the PACA on the CPU where
 * we're opening the segments, which can leave a stale copy in the
 * PACAs on other CPUs.
 *
 * This can be triggered either with multiple threads sharing the mm,
 * or with a single thread which is migrated from one CPU, to another
 * (where the mapping occurs), then back again (where we touch the
 * stale SLB).  We use the second method in this test, since it's
 * easier to force (using sched_setaffinity).  However it relies on a
 * close-to-idle system, if any process other than a kernel thread
 * runs on the first CPU between runs of the test process, the SLB
 * will be flushed and we won't trigger the bug, hence the
 * PASS_INCONCLUSIVE().  Obviously, this test won't work on a 1-cpu
 * system (should get CONFIG() on the sched_setaffinity()).
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>

#include <hugetlbfs.h>

#include "hugetests.h"

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p;
	volatile unsigned long *q;
	int err;
	cpu_set_t cpu0, cpu1;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	CPU_ZERO(&cpu0);
	CPU_SET(0, &cpu0);
	CPU_ZERO(&cpu1);
	CPU_SET(1, &cpu1);

	err = sched_setaffinity(getpid(), CPU_SETSIZE/8, &cpu0);
	if (err != 0)
		CONFIG("sched_setaffinity(cpu0): %s", strerror(errno));

	err = sched_setaffinity(getpid(), CPU_SETSIZE/8, &cpu1);
	if (err != 0)
		CONFIG("sched_setaffinity(): %s", strerror(errno));

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));

	err = sched_setaffinity(getpid(), CPU_SETSIZE/8, &cpu0);
	if (err != 0)
		CONFIG("sched_setaffinity(cpu0): %s", strerror(errno));

	q = (volatile unsigned long *)(p + getpagesize());
	*q = 0xdeadbeef;

	PASS_INCONCLUSIVE();
}

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <hugetlbfs.h>
#include "hugetests.h"

/*
 * Test rationale:
 *
 * On PowerPC, the address space is divided into segments.  These segments can
 * contain either huge pages or normal pages, but not both.  All segments are
 * initially set up to map normal pages.  When a huge page mapping is created
 * within a set of empty segments, they are "enabled" for huge pages at that
 * time.  Once enabled for huge pages, they can not be used again for normal
 * pages for the remaining lifetime of the process.
 *
 * If the segment immediately preceeding the segment containing the stack is
 * converted to huge pages and the stack is made to grow into the this
 * preceeding segment, some kernels may attempt to map normal pages into the
 * huge page-only segment -- resulting in bugs.
 *
 * The kernel bug in question was fixed by commit
 * 0d59a01bc461bbab4017ff449b8401151ef44cf6.
 */

#ifdef __LP64__
#define STACK_ALLOCATION_SIZE	(256*1024*1024)
#else
#define STACK_ALLOCATION_SIZE	(16*1024*1024)
#endif

void do_child(void *stop_address)
{
	struct rlimit r;
	volatile int *x;

	/* corefile from this process is not interesting and limiting
	 * its size can save a lot of time. '1' is a special value,
	 * that will also abort dumping via pipe, which by default
	 * sets limit to RLIM_INFINITY. */
	r.rlim_cur = 1;
	r.rlim_max = 1;
	setrlimit(RLIMIT_CORE, &r);

	do {
		x = alloca(STACK_ALLOCATION_SIZE);
		*x = 1;
	} while ((void *)x >= stop_address);
}

int main(int argc, char *argv[])
{
	int fd, pid, s, ret;
	struct rlimit r;
	char *b;
	long hpage_size = gethugepagesize();
	void *stack_address, *mmap_address, *heap_address;

	test_init(argc, argv);

	ret = getrlimit(RLIMIT_STACK, &r);
	if (ret)
		CONFIG("getrlimit failed: %s", strerror(errno));

	if (r.rlim_cur != RLIM_INFINITY)
		CONFIG("Stack rlimit must be 'unlimited'");

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		CONFIG("Couldn't get hugepage fd");

	stack_address = alloca(0);
	heap_address = sbrk(0);

	/*
	 * paranoia: start mapping two hugepages below the start of the stack,
	 * in case the alignment would cause us to map over something if we
	 * only used a gap of one hugepage.
	 */
	mmap_address = PALIGN(stack_address - 2 * hpage_size, hpage_size);

	do {
		b = mmap(mmap_address, hpage_size, PROT_READ|PROT_WRITE,
						MAP_FIXED|MAP_SHARED, fd, 0);
		mmap_address -= hpage_size;
		/*
		 * if we get all the way down to the heap, stop trying
		 */
		if (mmap_address <= heap_address)
			break;
	} while (b == MAP_FAILED);

	if (b == MAP_FAILED)
		FAIL("mmap: %s", strerror(errno));

	if ((pid = fork()) < 0)
		FAIL("fork: %s", strerror(errno));

	if (pid == 0) {
		do_child(mmap_address);
		exit(0);
	}

	ret = waitpid(pid, &s, 0);
	if (ret == -1)
		FAIL("waitpid: %s", strerror(errno));

	/*
	 * The child grows its stack until a failure occurs.  We expect
	 * this to result in a SIGSEGV.  If any other signal is
	 * delivered (ie. SIGTRAP) or no signal is sent at all, we
	 * determine the kernel has not behaved correctly and trigger a
	 * test failure.
	 */
	if (WIFSIGNALED(s)) {
		int sig = WTERMSIG(s);

		if (sig == SIGSEGV) {
			PASS();
		} else {
			FAIL("Got unexpected signal: %s", strsignal(sig));
		}
	}
	FAIL("Child not signalled");
}

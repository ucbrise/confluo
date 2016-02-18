/*
 * Test rationale:
 *
 * Some kernel versions after hugepage demand allocation was added
 * used a dubious heuristic to check if there was enough hugepage
 * space available for a given mapping.  The number of
 * not-already-instantiated pages in the mapping was compared against
 * the total hugepage free pool.  It was very easy to confuse this
 * heuristic into overcommitting by allocating hugepage memory in
 * chunks, each less than the total available pool size but together
 * more than available.  This would generally lead to OOM SIGKILLs of
 * one process or another when it tried to instantiate pages beyond
 * the available pool.
 *
 *
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
#include <sys/wait.h>
#include <signal.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define RANDOM_CONSTANT	0x1234ABCD

int main(int argc, char *argv[])
{
	long hpage_size;
	unsigned long totpages, chunk1, chunk2;
	int fd;
	void *p, *q;
	pid_t child, ret;
	int status;

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	totpages = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	chunk1 = (totpages / 2) + 1;
	chunk2 = totpages - chunk1 + 1;

	verbose_printf("overcommit: %ld hugepages available: "
		       "chunk1=%ld chunk2=%ld\n", totpages, chunk1, chunk2);

	p = mmap(NULL, chunk1*hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() chunk1: %s", strerror(errno));

	q = mmap(NULL, chunk2*hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, chunk1*hpage_size);
	if (q == MAP_FAILED) {
		if (errno != ENOMEM)
			FAIL("mmap() chunk2: %s", strerror(errno));
		else
			PASS();
	}

	verbose_printf("Looks like we've overcommitted, testing...\n");

	/* Looks like we're overcommited, but we need to confirm that
	 * this is bad.  We touch it all in a child process because an
	 * overcommit will generally lead to a SIGKILL which we can't
	 * handle, of course. */
	child = fork();
	if (child < 0)
		FAIL("fork(): %s", strerror(errno));

	if (child == 0) {
		memset(p, 0, chunk1*hpage_size);
		memset(q, 0, chunk2*hpage_size);
		exit(0);
	}

	ret = waitpid(child, &status, 0);
	if (ret < 0)
		FAIL("waitpid(): %s", strerror(errno));

	if (WIFSIGNALED(status))
		FAIL("Killed by signal \"%s\" due to overcommit",
		     strsignal(WTERMSIG(status)));

	PASS();
}

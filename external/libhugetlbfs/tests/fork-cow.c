/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 David Gibson, IBM Corporation.
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

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <hugetlbfs.h>
#include "hugetests.h"

/*
 * Test rationale:
 *
 * This checks copy-on-write semantics, specifically the semantics of
 * a MAP_PRIVATE mapping across a fork().  Some versions of the
 * powerpc kernel had a bug in huge_ptep_set_wrprotect() which would
 * fail to flush the hash table after setting the write protect bit in
 * the parent's page tables, thus allowing the parent to pollute the
 * child's mapping.
 */

#define RANDOM_CONSTANT		0x1234ABCD
#define OTHER_CONSTANT		0xfeef5678

/*
 * The parent uses this to check if the child terminated badly.
 */
static void sigchld_handler(int signum, siginfo_t *si, void *uc)
{
	if (WEXITSTATUS(si->si_status) != 0)
		FAIL("Child failed: %d", WEXITSTATUS(si->si_status));
	if (WIFSIGNALED(si->si_status))
		FAIL("Child recived signal %s",
			strsignal(WTERMSIG(si->si_status)));
}

int main(int argc, char ** argv)
{
	int fd, ret, status;
	void *syncarea;
	volatile unsigned int *p;
	volatile unsigned int *trigger, *child_readback;
	unsigned int parent_readback;
	long hpage_size;
	pid_t pid;
	struct sigaction sa = {
		.sa_sigaction = sigchld_handler,
		.sa_flags = SA_SIGINFO,
	};

	test_init(argc, argv);

	check_free_huge_pages(2);

	if (argc != 1)
		CONFIG("Usage: fork-cow\n");

	/* Get a shared normal page for synchronization */
	verbose_printf("Mapping synchronization area..");
	syncarea = mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE,
			MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if (syncarea == MAP_FAILED)
		FAIL("mmap() sync area: %s", strerror(errno));
	verbose_printf("done\n");

	trigger = syncarea;
	*trigger = 0;

	child_readback = trigger + 1;
	*child_readback = 0;

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		CONFIG("hugetlbfs_unlinked_fd() failed\n");

	verbose_printf("Mapping hugepage area...");
	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	verbose_printf("mapped at %p\n", p);

	/* Touch the page for write in parent */
	verbose_printf("Parent writes pre-fork...");
	*p = RANDOM_CONSTANT;
	verbose_printf("%x\n", RANDOM_CONSTANT);

	ret = sigaction(SIGCHLD, &sa, NULL);
	if (ret)
		FAIL("sigaction(): %s", strerror(errno));

	if ((pid = fork()) < 0)
		FAIL("fork(): %s", strerror(errno));

	if (pid != 0) {
		/* Parent */
		verbose_printf("Parent writes post-fork...");
		*p = ~RANDOM_CONSTANT;
		verbose_printf("%x\n", ~RANDOM_CONSTANT);

		*trigger = 1;

		while (*trigger != 2)
			;

		verbose_printf("Parent reads..");
		parent_readback = *p;
		verbose_printf("%x\n", parent_readback);

		*trigger = 3;
	} else {
		/* Child */
		verbose_printf("Child starts..\n");

		while (*trigger != 1)
			;

		verbose_printf("Child reads...");
		*child_readback = *p;
		verbose_printf("%x\n", *child_readback);

		verbose_printf("Child writes...");
		*p = OTHER_CONSTANT;
		verbose_printf("%x\n", OTHER_CONSTANT);

		*trigger = 2;

		while (*trigger != 3)
			;

		verbose_printf("Child exits...\n");
		exit(0);
	}

	verbose_printf("child_readback = 0x%x, parent_readback = 0x%x\n",
		       *child_readback, parent_readback);

	if (*child_readback != RANDOM_CONSTANT)
		FAIL("Child read back 0x%x instead of 0x%x",
		     *child_readback, RANDOM_CONSTANT);
	if (parent_readback != ~RANDOM_CONSTANT)
		FAIL("Parent read back 0x%x instead of 0x%x",
		     parent_readback, RANDOM_CONSTANT);

	ret = waitpid(pid, &status, 0);
	if (ret < 0)
		FAIL("waitpid(): %s", strerror(errno));

	PASS();
}

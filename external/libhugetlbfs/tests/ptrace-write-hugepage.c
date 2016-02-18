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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define CONST	0xdeadbeefL

static long hpage_size;
static volatile int ready_to_trace = 0;

static void sigchld_handler(int signum, siginfo_t *si, void *uc)
{
	int status;

	wait(&status);
	if (WIFEXITED(status))
		exit(WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		exit(status);

	ready_to_trace = 1;
}

static void child(int hugefd, int pipefd)
{
	void *p;
	int err;

	p = mmap(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 hugefd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));

	memset(p, 0, hpage_size);

	verbose_printf("Child mapped data at %p\n", p);

	err = write(pipefd, &p, sizeof(p));
	if (err == -1)
		FAIL("Writing to pipe: %s", strerror(errno));
	if (err != sizeof(p))
		FAIL("Short write to pipe");

	pause();
}

static void do_poke(pid_t pid, void *p)
{
	long err;

	verbose_printf("Poking...");
	err = ptrace(PTRACE_POKEDATA, pid, p, (void *)CONST);
	if (err)
		FAIL("ptrace(POKEDATA): %s", strerror(errno));
	verbose_printf("done\n");

	verbose_printf("Peeking...");
	err = ptrace(PTRACE_PEEKDATA, pid, p, NULL);
	if (err == -1)
		FAIL("ptrace(PEEKDATA): %s", strerror(errno));

	if (err != CONST)
		FAIL("mismatch (%lx instead of %lx)", err, CONST);
	verbose_printf("done\n");
}

int main(int argc, char *argv[])
{
	int fd;
	int pipefd[2];
	long err;
	pid_t cpid;
	void *p;
	struct sigaction sa = {
		.sa_sigaction = sigchld_handler,
		.sa_flags = SA_SIGINFO,
	};
	struct sigaction old_sa;


	test_init(argc, argv);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	err = sigaction(SIGCHLD, &sa, &old_sa);
	if (err)
		FAIL("Can't install SIGCHLD handler: %s", strerror(errno));

	err = pipe(pipefd);
	if (err)
		FAIL("pipe(): %s", strerror(errno));

	cpid = fork();
	if (cpid < 0)
		FAIL("fork(): %s", strerror(errno));


	if (cpid == 0) {
		child(fd, pipefd[1]);
		exit(0);
	}

	/* Parent */
	err = read(pipefd[0], &p, sizeof(p));
	if (err == -1)
		FAIL("Reading pipe: %s\n", strerror(errno));
	if (err != sizeof(p))
		FAIL("Short read over pipe");

	verbose_printf("Parent received address %p\n", p);

	err = ptrace(PTRACE_ATTACH, cpid, NULL, NULL);
	if (err)
		FAIL("ptrace(ATTACH): %s", strerror(errno));

	while (! ready_to_trace)
		;

	do_poke(cpid, p);
	do_poke(cpid, p + getpagesize());

	err = sigaction(SIGCHLD, &old_sa, NULL);
	if (err)
		FAIL("Clearing SIGCHLD handler: %s", strerror(errno));

	ptrace(PTRACE_KILL, cpid, NULL, NULL);

	PASS();
}

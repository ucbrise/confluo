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
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <hugetlbfs.h>
#include "hugetests.h"

#define P "shm-fork"
#define DESC \
	"* Test shared memory behavior when multiple threads are attached  *\n"\
	"* to a segment.  A segment is created and then children are       *\n"\
	"* spawned which attach, write, read (verify), and detach from the *\n"\
	"* shared memory segment.                                          *"

extern int errno;

/* Global Configuration */
static int nr_hugepages;
static int numprocs;
static int shmid = -1;

#define MAX_PROCS 200
#define BUF_SZ 256

#define CHILD_FAIL(thread, fmt, ...) \
	do { \
		verbose_printf("Thread %d (pid=%d) FAIL: " fmt, \
			       thread, getpid(), __VA_ARGS__); \
		exit(1); \
	} while (0)

void cleanup(void)
{
	remove_shmid(shmid);
}

static void do_child(int thread, unsigned long size)
{
	volatile char *shmaddr;
	int j, k;

	verbose_printf(".");
	for (j=0; j<5; j++) {
		shmaddr = shmat(shmid, 0, SHM_RND);
		if (shmaddr == MAP_FAILED)
			CHILD_FAIL(thread, "shmat() failed: %s",
				   strerror(errno));

		for (k=0;k<size;k++)
			shmaddr[k] = (char) (k);
		for (k=0;k<size;k++)
			if (shmaddr[k] != (char)k)
				CHILD_FAIL(thread, "Index %d mismatch", k);

		if (shmdt((const void *)shmaddr) != 0)
			CHILD_FAIL(thread, "shmdt() failed: %s",
				   strerror(errno));
	}
	exit(0);
}

int main(int argc, char ** argv)
{
	unsigned long size;
	long hpage_size;
	int pid, status;
	int i;
	int wait_list[MAX_PROCS];

	test_init(argc, argv);

	if (argc < 3)
		CONFIG("Usage:  %s <# procs> <# pages>", argv[0]);

	numprocs = atoi(argv[1]);
	nr_hugepages = atoi(argv[2]);

	if (numprocs > MAX_PROCS)
		CONFIG("Cannot spawn more than %d processes", MAX_PROCS);

	check_hugetlb_shm_group();

	hpage_size = check_hugepagesize();
        size = hpage_size * nr_hugepages;
	verbose_printf("Requesting %lu bytes\n", size);
	if ((shmid = shmget(2, size, SHM_HUGETLB|IPC_CREAT|SHM_R|SHM_W )) < 0)
		FAIL("shmget(): %s", strerror(errno));

	verbose_printf("shmid: %d\n", shmid);

	verbose_printf("Spawning children:\n");
	for (i=0; i<numprocs; i++) {
		if ((pid = fork()) < 0)
			FAIL("fork(): %s", strerror(errno));

		if (pid == 0)
			do_child(i, size);

		wait_list[i] = pid;
	}

	for (i=0; i<numprocs; i++) {
		waitpid(wait_list[i], &status, 0);
		if (WEXITSTATUS(status) != 0)
			FAIL("Thread %d (pid=%d) failed", i, wait_list[i]);

		if (WIFSIGNALED(status))
			FAIL("Thread %d (pid=%d) received unhandled signal",
			     i, wait_list[i]);
	}

	PASS();
}

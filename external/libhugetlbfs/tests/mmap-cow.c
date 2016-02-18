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

extern int errno;

#define P "mmap-cow"
#define DESC \
	"* Tests copy-on-write semantics of large pages where a number     *\n"\
	"* of threads map the same file with the MAP_PRIVATE flag.  The    *\n"\
	"* threads then write into their copy of the mapping and recheck   *\n"\
	"* the contents to ensure they were not corrupted by the other     *\n"\
	"* threads.                                                        *"\

#define HTLB_FILE "mmap-cow"
#define BUF_SZ 256

#define CHILD_FAIL(thread, fmt, ...) \
	do { \
		verbose_printf("Thread %d (pid=%d) FAIL: " fmt, \
			       thread, getpid(), __VA_ARGS__); \
		exit(1); \
	} while (0)

/* Setup Configuration */
static int nr_hugepages;	/* Number of huge pages to allocate */
static unsigned int threads;	/* Number of threads to run */

static int mmap_file(int fd, char **addr, size_t size, int type)
{
	int flags = 0;

	*addr = mmap(NULL, size, PROT_READ|PROT_WRITE, flags | type, fd, 0);
	if (*addr == MAP_FAILED)
		return -1;

	return 0;
}

static void do_work(int thread, size_t size, int fd)
{
	char *addr;
	size_t i;
	char pattern = thread+65;

	if (mmap_file(fd, &addr, size, MAP_PRIVATE))
		CHILD_FAIL(thread, "mmap() failed: %s", strerror(errno));

	verbose_printf("Thread %d (pid=%d): Mapped at address %p\n",
		       thread, getpid(), addr);

	/* Write to the mapping with a distinct pattern */
	verbose_printf("Thread %d (pid=%d): Writing %c to the mapping\n",
		       thread, getpid(), pattern);
	for (i = 0; i < size; i++)
		memcpy((char *)addr+i, &pattern, 1);

	if (msync(addr, size, MS_SYNC))
		CHILD_FAIL(thread, "msync() failed: %s", strerror(errno));

	/* Verify the pattern */
	for (i = 0; i < size; i++)
		if (addr[i] != pattern)
			CHILD_FAIL(thread, "Corruption at %p; "
				   "Got %c, Expected %c",
				   &addr[i], addr[i], pattern);

	verbose_printf("Thread %d (pid=%d): Pattern verified\n",
		       thread, getpid());

	/* Munmap the area */
	munmap(addr, size);
	close(fd);
	exit(0);
}

int main(int argc, char ** argv)
{
	char *addr;
	long hpage_size;
	size_t size;
	int i, pid, status, fd, ret;
	pid_t *wait_list;

	test_init(argc, argv);

	if (argc < 3)
		CONFIG("Usage: mmap-cow <# threads> <# pages>\n");

	nr_hugepages = atoi(argv[2]);
	threads = atoi(argv[1]);

	if ((threads+1) > nr_hugepages)
		CONFIG("Need more hugepages than threads\n");

	wait_list = malloc(threads * sizeof(pid_t));
	if (wait_list == NULL)
		CONFIG("Couldn't allocate memory for wait_list\n");

	hpage_size = check_hugepagesize();
	/* Have to have enough available hugepages for each thread to
	 * get its own copy, plus one for the parent/page-cache */
	size = (nr_hugepages / (threads+1)) * hpage_size;
	verbose_printf("hpage_size is %lx, Size is %zu, threads: %u\n",
		       hpage_size, size, threads);

	/* First, open the file */
	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		CONFIG("hugetlbfs_unlinked_fd() failed: %s\n",
		       strerror(errno));

	/* First, mmap the file with MAP_SHARED and fill with data
	 * If this is not done, then the fault handler will not be
	 * called in the kernel since private mappings will be
	 * created for the children at prefault time.
	 */
	if (mmap_file(fd, &addr, size, MAP_SHARED))
		FAIL("Failed to create shared mapping: %s", strerror(errno));

	for (i = 0; i < size; i += 8) {
		memcpy(addr+i, "deadbeef", 8);
	}

	for (i=0; i<threads; i++) {
		if ((pid = fork()) < 0)
			FAIL("fork: %s", strerror(errno));

		if (pid == 0)
			do_work(i, size, fd);

		wait_list[i] = pid;
	}
	for (i=0; i<threads; i++) {
		ret = waitpid(wait_list[i], &status, 0);
		if (ret < 0)
			FAIL("waitpid(): %s", strerror(errno));
		if (WEXITSTATUS(status) != 0)
			FAIL("Thread %d (pid=%d) failed", i, wait_list[i]);

		if (WIFSIGNALED(status))
			FAIL("Thread %d (pid=%d) received unhandled signal", i,
			     wait_list[i]);
	}

	munmap(addr, size);
	close(fd);
	free(wait_list);

	PASS();
}

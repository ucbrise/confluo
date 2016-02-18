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
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <hugetlbfs.h>
#include <sys/vfs.h>
#include "hugetests.h"

/*
 * Test Rationale:
 *
 * The number of global huge pages available to a mounted hugetlbfs filesystem
 * can be limited using a fs quota mechanism by setting the size attribute at
 * mount time.  Older kernels did not properly handle quota accounting in a
 * number of cases (eg. for MAP_PRIVATE pages, and wrt MAP_SHARED reservation.
 *
 * This test replays some scenarios on a privately mounted filesystem to check
 * for regressions in hugetlbfs quota accounting.
 */

extern int errno;

#define BUF_SZ 1024

/* Global test configuration */
static long hpage_size;
char *mountpoint = NULL;

/* map action flags */
#define ACTION_COW		0x0001
#define ACTION_TOUCH		0x0002

/* Testlet results */
#define GOOD 0
#define BAD_SIG  1
#define BAD_EXIT 2

char result_str[3][10] = { "pass", "killed", "fail" };

void cleanup(void)
{
	if (mountpoint && (umount(mountpoint) == 0))
		rmdir(mountpoint);
}

/*
 * Debugging function:  Verify the counters in the hugetlbfs superblock that
 * are used to implement the filesystem quotas.
 */
void _verify_stat(int line, long tot, long free, long avail)
{
	struct statfs s;
	statfs(mountpoint, &s);

	if (s.f_blocks != tot || s.f_bfree != free || s.f_bavail != avail)
		FAIL("Bad quota counters at line %i: total: %li free: %li "
		       "avail: %li\n", line, s.f_blocks, s.f_bfree, s.f_bavail);
}
#define verify_stat(t, f, a) _verify_stat(__LINE__, t, f, a)

void get_quota_fs(unsigned long size, char *prog)
{
	char mount_str[17];
	char mount_opts[50];
	int nr_written;

	nr_written = snprintf(mount_opts, 20, "size=%luK", size/1024);

	/*
	 * If the mount point now in use does not use the system default
	 * huge page size, specify the desired size when mounting.  When
	 * the sizes do match, we avoid specifying the pagesize= option to
	 * preserve backwards compatibility with kernels that do not
	 * recognize that option.
	 */
	if (!using_system_hpage_size(hugetlbfs_find_path()))
		snprintf(mount_opts + nr_written, 29, ",pagesize=%lu",
			hpage_size);

	sprintf(mount_str, "/tmp/huge-XXXXXX");
	if (!mkdtemp(mount_str))
		FAIL("Cannot create directory for mountpoint: %s",
							strerror(errno));

	if (mount("none", mount_str, "hugetlbfs", 0, mount_opts)) {
		perror("mount");
		FAIL();
	}
	mountpoint = mount_str;

	/*
	 * Set HUGETLB_PATH and then exec the test again.  This will cause
	 * libhugetlbfs to use this newly created mountpoint.
	 */
	if (setenv("HUGETLB_PATH", mount_str, 1))
		FAIL("Cannot set HUGETLB_PATH environment variable: %s",
							strerror(errno));
	verbose_printf("Using %s as temporary mount point.\n", mount_str);

	execlp(prog, prog, "-p", mount_str, NULL);
	FAIL("execle failed: %s", strerror(errno));
}

void map(unsigned long size, int mmap_flags, int action_flags)
{
	int fd;
	char *a, *b, *c;

	fd = hugetlbfs_unlinked_fd();
	if (!fd) {
		verbose_printf("hugetlbfs_unlinked_fd () failed\n");
		exit(1);
	}

	a = mmap(0, size, PROT_READ|PROT_WRITE, mmap_flags, fd, 0);
	if (a == MAP_FAILED) {
		verbose_printf("mmap failed: %s\n", strerror(errno));
		exit(1);
	}


	if (action_flags & ACTION_TOUCH)
		for (b = a; b < a + size; b += hpage_size)
			*(b) = 1;

	if (action_flags & ACTION_COW) {
		c = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
		if (c == MAP_FAILED) {
			verbose_printf("Creating COW mapping failed: %s\n", strerror(errno));
			exit(1);
		}
		if ((*c) !=  1) {
			verbose_printf("Data mismatch when setting up COW");
			exit(1);
		}
		(*c) = 0;
		munmap(c, size);
	}

	munmap(a, size);
	close(fd);
}

void do_unexpected_result(int line, int expected, int actual)
{
	FAIL("Unexpected result on line %i: expected %s, actual %s",
		line, result_str[expected], result_str[actual]);
}

void _spawn(int l, int expected_result, unsigned long size, int mmap_flags,
							int action_flags)
{
	pid_t pid;
	int status;
	int actual_result;

	pid = fork();
	if (pid == 0) {
		map(size, mmap_flags, action_flags);
		exit(0);
	} else if (pid < 0) {
		FAIL("fork(): %s", strerror(errno));
	} else {
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) == 0)
				actual_result = GOOD;
			else
				actual_result = BAD_EXIT;
		} else {
			actual_result = BAD_SIG;
		}

		if (actual_result != expected_result)
			do_unexpected_result(l, expected_result, actual_result);
	}
}
#define spawn(e,s,mf,af) _spawn(__LINE__, e, s, mf, af)

int main(int argc, char ** argv)
{
	int private_resv;
	int bad_priv_resv;

	test_init(argc, argv);
	hpage_size = check_hugepagesize();

	if ((argc == 3) && !strcmp(argv[1], "-p"))
		mountpoint = argv[2];
	else
		get_quota_fs(hpage_size, argv[0]);

	check_must_be_root();
	check_free_huge_pages(1);

	private_resv = kernel_has_private_reservations();
	if (private_resv == -1)
		FAIL("kernel_has_private_reservations() failed\n");
	bad_priv_resv = private_resv ? BAD_EXIT : BAD_SIG;

	/*
	 * Check that unused quota is cleared when untouched mmaps are
	 * cleaned up.
	 */
	spawn(GOOD, hpage_size, MAP_PRIVATE, 0);
	verify_stat(1, 1, 1);
	spawn(GOOD, hpage_size, MAP_SHARED, 0);
	verify_stat(1, 1, 1);

	/*
	 * Check that simple page instantiation works within quota limits
	 * for private and shared mappings.
	 */
	spawn(GOOD, hpage_size, MAP_PRIVATE, ACTION_TOUCH);
	spawn(GOOD, hpage_size, MAP_SHARED, ACTION_TOUCH);

	/*
	 * Page instantiation should be refused if doing so puts the fs
	 * over quota.
	 */
	spawn(BAD_EXIT, 2 * hpage_size, MAP_SHARED, ACTION_TOUCH);

	/*
	 * If private mappings are reserved, the quota is checked up front
	 * (as is the case for shared mappings).
	 */
	spawn(bad_priv_resv, 2 * hpage_size, MAP_PRIVATE, ACTION_TOUCH);

	/*
	 * COW should not be allowed if doing so puts the fs over quota.
	 */
	spawn(bad_priv_resv, hpage_size, MAP_SHARED, ACTION_TOUCH|ACTION_COW);
	spawn(bad_priv_resv, hpage_size, MAP_PRIVATE, ACTION_TOUCH|ACTION_COW);

	/*
	 * Make sure that operations within the quota will succeed after
	 * some failures.
	 */
	spawn(GOOD, hpage_size, MAP_SHARED, ACTION_TOUCH);
	spawn(GOOD, hpage_size, MAP_PRIVATE, ACTION_TOUCH);

	PASS();
}

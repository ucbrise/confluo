/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 Eric Munson, IBM Corporation.
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
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <errno.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define BUF_SIZE 4096
#define FILLER "tmpfs /var/run tmpfs rw,nosuid,nodev,noexec,mode=755 0 0\n"

int in_test; /* = 0; */
int tmp_mounts_fd; /* = 0; */
FILE *tmp_stream; /* = NULL; */

/*
 * We override the normal open, so we can remember the fd for the
 * mounts file
 */
int open(const char *path, int flags, ...)
{
	int (*old_open)(const char *, int, ...);
	int fd;
	va_list ap;

	old_open = dlsym(RTLD_NEXT, "open");
	if (in_test && strcmp(path, "/proc/mounts") == 0)
		return tmp_mounts_fd;
	va_start(ap, flags);
	fd = (old_open)(path, flags, va_arg(ap, mode_t));
	va_end(ap);
	return fd;
}

void make_test_mounts()
{
	char buf[BUF_SIZE];
	int mounts_fd;
	unsigned int written = 0;
	int ret;
	int filler_sz;

	mounts_fd = open("/proc/mounts", O_RDONLY);
	if (mounts_fd < 0)
		FAIL("Unable to open /proc/mounts: %s", strerror(errno));
	tmp_stream = tmpfile();
	if (!tmp_stream)
		FAIL("Unable to open temporary mounts file: %s", strerror(errno));

	tmp_mounts_fd = fileno(tmp_stream);
	if (tmp_mounts_fd < 0)
		FAIL("Unable to get file descriptor from stream.");

	filler_sz = strlen(FILLER);

	while (written < BUF_SIZE) {
		if (write(tmp_mounts_fd, FILLER, filler_sz) < 0)
			FAIL("Unable to write to temp mounts file: %s",
				strerror(errno));
		written += filler_sz;
	}

	while ((ret = read(mounts_fd, buf, BUF_SIZE)) > 0)
		if (write(tmp_mounts_fd, buf, ret) < 0)
			FAIL("Unable to write to temp mounts file: %s",
				strerror(errno));

	close(mounts_fd);
	if (lseek(tmp_mounts_fd, 0, SEEK_SET) < 0)
		FAIL("Unable to move temp mounts stream to beginning of file: %s",
			strerror(errno));
}

int main(int argc, char *argv[])
{
	int fd;

	make_test_mounts();
	test_init(argc, argv);
	in_test = 1;

	fd = hugetlbfs_unlinked_fd();

	fclose(tmp_stream);
	if (fd < 0)
		FAIL("Unable to find mount point\n");

	PASS();
}

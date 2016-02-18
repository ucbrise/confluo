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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>

#include <hugetlbfs.h>

#include "hugetests.h"

/* We override the normal open, so libhugetlbfs gets a /proc/meminfo
 * which doesn't contain any hugepage information */
int open(const char *path, int flags, ...)
{
	int (*old_open)(const char *, int, ...);
	int fd;

	if (strcmp(path, "/proc/meminfo") == 0) {
		FILE *f;

		f = popen("/bin/grep -vi ^hugepage /proc/meminfo", "r");
		return fileno(f);
	}

	if (strcmp(path, "/proc/mounts") == 0) {
		FILE *f;

		f = popen("/bin/grep -vi hugetlbfs /proc/mounts", "r");
		return fileno(f);
	}

	old_open = dlsym(RTLD_NEXT, "open");
	if (flags & O_CREAT) {
		va_list ap;

		va_start(ap, flags);
		fd = (*old_open)(path, flags, va_arg(ap, mode_t));
		va_end(ap);
		return fd;
	} else {
		return (*old_open)(path, flags);
	}
}

int main(int argc, char *argv[])
{
	long hpage_size;

	test_init(argc, argv);

	hpage_size = gethugepagesize();
	if (hpage_size == -1)
		PASS();

	FAIL("Mysteriously found a hugepage size of %ld\n", hpage_size);
}

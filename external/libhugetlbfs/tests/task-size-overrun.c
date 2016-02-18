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

#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define MAPS_BUF_SZ 4096

static unsigned long find_last_mapped(void)
{
	FILE *f;
	char line[MAPS_BUF_SZ];
	char *tmp;
	unsigned long start, end, off, ino;
	int ret;

	f = fopen("/proc/self/maps", "r");
	if (!f) {
		ERROR("Failed to open /proc/self/maps: %s\n", strerror(errno));
		return -1;
	}

	do {
		tmp = fgets(line, MAPS_BUF_SZ, f);
	} while (tmp);
	fclose(f);

	verbose_printf("Last map: %s", line);
	ret = sscanf(line, "%lx-%lx %*s %lx %*s %ld %*s", &start, &end, &off, &ino);
	if (ret == EOF)
		FAIL("Couldn't parse /proc/self/maps line: %s: %s\n", line,
							strerror(errno));
	if (ret != 4)
		FAIL("Couldn't parse /proc/self/maps line: %s\n", line);

	verbose_printf("Last map at 0x%lx-0x%lx\n", start, end);
	return end;
}

static unsigned long find_task_size(void)
{
	unsigned long addr;
	void *p;

	addr = find_last_mapped();
	if (!addr || ((addr % getpagesize()) != 0))
		FAIL("Bogus stack end address, 0x%lx!?", addr);

	while (addr) {
		p = mmap64((void *)addr, getpagesize(), PROT_READ,
			   MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
		if (p == MAP_FAILED) {
			verbose_printf("Searching map failed: %s\n", strerror(errno));
			return addr;
		}
		munmap(p, getpagesize());
		addr += getpagesize();
#if defined(__s390x__)
		if (addr > (1UL << 42) && addr < (1UL << 53))
			addr = 1UL << 53;
#endif
	}
	/* addr wrapped around */
	return 0;
}

int main(int argc, char *argv[])
{
	long hpage_size;
	int fd;
	void *p;
	unsigned long task_size;
	unsigned long straddle_addr;

	test_init(argc, argv);

	task_size = find_task_size();

	verbose_printf("TASK_SIZE = 0x%lx\n", task_size);

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	straddle_addr = task_size - hpage_size;
	straddle_addr = ALIGN(straddle_addr, hpage_size);

	/* We first try to get the mapping without MAP_FIXED */
	verbose_printf("Mapping without MAP_FIXED at %lx...", straddle_addr);
	errno = 0;
	p = mmap((void *)straddle_addr, 2*hpage_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED, fd, 0);
	verbose_printf("%s\n", strerror(errno));
	if (p == (void *)straddle_addr)
		FAIL("Apparently suceeded in mapping across TASK_SIZE boundary");

	verbose_printf("Mapping with MAP_FIXED at %lx...", straddle_addr);
	errno = 0;
	p = mmap((void *)straddle_addr, 2*hpage_size, PROT_READ|PROT_WRITE,
		 MAP_SHARED|MAP_FIXED, fd, 0);
	verbose_printf("%s\n", strerror(errno));
	if (p != MAP_FAILED)
		FAIL("Apparently suceeded in mapping across TASK_SIZE boundary");

	PASS();
}

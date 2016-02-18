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
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <hugetlbfs.h>
#include "hugetests.h"

extern int errno;

/* Global Configuration */
#define P "shm-getraw"
#define DESC \
	"* This test exercizes the code path which performs raw device IO  *\n"\
	"* into a large page backed shared memory segment.  The specified  *\n"\
	"* device will be read into a shared memory segment.               *"

static int nr_hugepages;
static int shmid = -1;

void cleanup(void)
{
	remove_shmid(shmid);
}

int main(int argc, char ** argv)
{
	size_t size;
	size_t i;
	long hpage_size = check_hugepagesize();
	volatile char *shmaddr;
	char *buffer;
	int raw_fd;

	test_init(argc, argv);

	check_hugetlb_shm_group();

	if (argc < 3)
		CONFIG("Usage:  %s <# pages> <device>", argv[0]);

	nr_hugepages = atoi(argv[1]);

	verbose_printf("hpage_size is: %ld\n", hpage_size);

	buffer = malloc(hpage_size*sizeof(char));
	if (!buffer)
		FAIL("malloc(%li)", hpage_size*sizeof(char));

	raw_fd = open(argv[2], O_RDONLY);
	if (!raw_fd)
		CONFIG("Cannot open raw device: %s", strerror(errno));

	size = hpage_size * nr_hugepages;

	verbose_printf("Requesting %zu bytes\n", size);

	if ((shmid = shmget(2, size, SHM_HUGETLB|IPC_CREAT|SHM_R|SHM_W )) < 0)
		FAIL("shmget(): %s", strerror(errno));

	verbose_printf("shmid: 0x%x\n", shmid);
	shmaddr = shmat(shmid, 0, SHM_RND) ;
	if (shmaddr == MAP_FAILED)
		FAIL("shmat() failed: %s", strerror(errno));

	verbose_printf("shmaddr: %p\n", shmaddr);

	/* Read a page from device and write to shm segment */
	for (i = 0; i < size; i+=hpage_size) {
		if (!read(raw_fd, buffer, hpage_size))
			FAIL("Can't read from raw device: %s",
			     strerror(errno));
		memcpy((char*)(shmaddr + i), buffer, hpage_size);
	}

	verbose_printf("Done.\n");
	if (shmdt((const void *)shmaddr) != 0)
		FAIL("shmdt() failed: %s", strerror(errno));

	free(buffer);
	PASS();
}

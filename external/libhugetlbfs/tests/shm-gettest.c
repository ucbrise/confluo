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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hugetlbfs.h>
#include "hugetests.h"

extern int errno;

/* Global Configuration */
#define P "shm-gettest"
#define DESC \
	"* A looping test to verify the functionality of large page backed *\n"\
	"* shared memory segments.  A segment is created, written,         *\n"\
	"* verified, and detached a specified number of times.             *"

static unsigned int iter;
static int nr_hugepages;
static int shmid = -1;
static long hpage_size;

void cleanup(void)
{
	remove_shmid(shmid);
}

static void do_one(size_t size) {
	size_t i,j;
	char pattern;
	char *shmaddr;

	verbose_printf("Requesting %zu bytes\n", size);

	if ((shmid = shmget(2, size, SHM_HUGETLB|IPC_CREAT|SHM_R|SHM_W )) < 0)
		FAIL("shmget(): %s", strerror(errno));

	verbose_printf("shmid: 0x%x\n", shmid);

	shmaddr = shmat(shmid, 0, SHM_RND) ;
	if (shmaddr == MAP_FAILED)
		FAIL("shmat(): %s", strerror(errno));

	verbose_printf("shmaddr: %p\n", shmaddr);

	for (i = 0; i < nr_hugepages; i++) {
		pattern = 65+(i%26);
		verbose_printf("Touching %p with %c\n", shmaddr+(i*hpage_size),pattern);
		memset(shmaddr+(i*hpage_size), pattern, hpage_size);
	}

	for (i = 0; i < nr_hugepages; i++) {
		pattern = 65+(i%26);
		verbose_printf("Verifying %p\n", (shmaddr+(i*hpage_size)));
		for (j = 0; j < hpage_size; j++)
			if (*(shmaddr+(i*hpage_size)+j) != pattern)
				FAIL("Verifying the segment failed. "
				     "Got %c, expected %c",
				     *(shmaddr+(i*hpage_size)+j), pattern);
	}

	if (shmdt((const void *)shmaddr) != 0)
		FAIL("shmdt(): %s", strerror(errno));
}

int main(int argc, char ** argv)
{
	size_t size;
	int i;

	test_init(argc, argv);

	if (argc < 3)
		CONFIG("Usage: shmgettest <# iterations> <# pages>\n");

	check_hugetlb_shm_group();

	iter = atoi(argv[1]);
	nr_hugepages = atoi(argv[2]);

	hpage_size = gethugepagesize();
	size = nr_hugepages * hpage_size;

	for (i=0; i < iter; i++)
		do_one(size);

	PASS();
}

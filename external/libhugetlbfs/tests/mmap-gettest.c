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
#include <sys/stat.h>
#include <fcntl.h>
#include <hugetlbfs.h>
#include "hugetests.h"

#define P "mmap-gettest"
#define DESC \
	"* This baseline test validates that a mapping of a certain size   *\n"\
	"* can be created, correctly.  Once created, all the pages are     *\n"\
	"* filled with a pattern and rechecked to test for corruption.     *\n"\
	"* The mapping is then released.  This process is repeated for a   *\n"\
	"* specified number of iterations.                                 *"

extern int errno;

#define BUF_SZ 256

/* Global test configuration */
#define HTLB_FILE "mmap-gettest"
static char hugetlb_mount[BUF_SZ];
static unsigned int iter;
static int nr_hugepages;
static long hpage_size;

static int do_one(char *mountpoint, size_t size) {
	char *ma;
	int fha;
	size_t i,j;
	char pattern = 'A';

	fha = hugetlbfs_unlinked_fd();
	if (fha < 0)
		CONFIG("Unable to open temp file in hugetlbfs (%s)",
		       strerror(errno));

	/* Map the files with MAP_PRIVATE */
	ma = mmap(NULL, size, (PROT_READ|PROT_WRITE), MAP_SHARED, fha, 0);
	if (ma == MAP_FAILED)
		FAIL("Failed to mmap the hugetlb file: %s", strerror(errno));

	/* Make sure the page is zeroed */
	for (i = 0; i < nr_hugepages; i++) {
		verbose_printf("Verifying %p\n", (ma+(i*hpage_size)));
		for (j = 0; j < hpage_size; j++) {
			if (*(ma+(i*hpage_size)+j) != 0)
				FAIL("Verifying the mmap area failed. "
				     "Got %c, expected 0",
				     *(ma+(i*hpage_size)+j));
		}
	}
	/* Fill each file with a pattern */
	for (i = 0; i < nr_hugepages; i++) {
		pattern = 65+(i%26);
		verbose_printf("Touching %p with %c\n", ma+(i*hpage_size),pattern);
		memset(ma+(i*hpage_size), pattern, hpage_size);
	}

	/* Verify the pattern */
	for (i = 0; i < nr_hugepages; i++) {
		pattern = 65+(i%26);
		verbose_printf("Verifying %p\n", (ma+(i*hpage_size)));
		for (j = 0; j < hpage_size; j++) {
			if (*(ma+(i*hpage_size)+j) != pattern)
				FAIL("Verifying the mmap area failed. "
				     "Got %c, expected %c",
				     *(ma+(i*hpage_size)+j),pattern);
		}
	}

	/* Munmap the area */
	munmap(ma, size);

	/* Close and delete the file */
	close(fha);
	return 0;
}

int main(int argc, char ** argv)
{
	size_t size;
	int i;

	test_init(argc, argv);

	if (argc < 3)
		CONFIG("Usage: %s <# iterations> <# pages>\n", argv[0]);

	iter = atoi(argv[1]);
	nr_hugepages = atoi(argv[2]);

	hpage_size = check_hugepagesize();
	size = nr_hugepages * hpage_size;

	for (i=0; i < iter; i++) {
		verbose_printf("Iteration %d\n", i);
		do_one(hugetlb_mount, size);
	}

	PASS();
}

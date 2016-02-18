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
#include <unistd.h>
#include <sys/mman.h>

#include <hugetlbfs.h>

#include "hugetests.h"

long hpage_size;
long oc_hugepages = -1;

/* Restore nr_overcommit_hugepages */
void cleanup(void)
{
	if (oc_hugepages != -1)
		set_nr_overcommit_hugepages(hpage_size, oc_hugepages);
}

/* Confirm a region really frees, only really important for GHP_FALLBACK */
void free_and_confirm_region_free(void *p, int line) {
	unsigned char vec = 0;
	free_huge_pages(p);
	if (mincore(p, 4, &vec) == 0 || vec)
		FAIL("free_huge_pages did not free region at line %d", line);
}

void test_get_huge_pages(int num_hugepages)
{
	unsigned long long mapping_size;
	void *p = get_huge_pages(num_hugepages * hpage_size, GHP_DEFAULT);
	if (p == NULL)
		FAIL("get_huge_pages() for %d hugepages", num_hugepages);

	memset(p, 1, hpage_size);

	mapping_size = get_mapping_page_size(
			(void *)p + (num_hugepages -1) * hpage_size);
	if (mapping_size != hpage_size)
		FAIL("Returned page is not hugepage");

	free_and_confirm_region_free(p, __LINE__);
	mapping_size = get_mapping_page_size(
			(void *)p + (num_hugepages -1) * hpage_size);
	if (mapping_size)
		FAIL("hugepage was not correctly freed");
}

int main(int argc, char *argv[])
{
	test_init(argc, argv);
	hpage_size = gethugepagesize();
	check_free_huge_pages(4);
	test_get_huge_pages(1);
	test_get_huge_pages(4);

	PASS();
}

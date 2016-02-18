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

/* Confirm a region really frees, only really important for GHR_FALLBACK */
void free_and_confirm_region_free(void *p, int line) {
	unsigned char vec = 0;
	free_hugepage_region(p);
	if (mincore(p, 4, &vec) == 0 || vec)
		FAIL("free_hugepage_region did not free region at line %d", line);
}

int test_unaligned_addr_huge(void *p)
{
	unsigned long long mapping_size;
	p = (void *)((unsigned long)p & ~((gethugepagesize()) - 1));
	mapping_size = get_mapping_page_size(p);
	return (mapping_size == hpage_size);
}

#define TESTLEN ((num_hugepages - 1) * hpage_size + hpage_size / 2)

void test_GHR_STRICT(int num_hugepages)
{
	int err;
	void *p = get_hugepage_region(TESTLEN, GHR_DEFAULT);
	if (p == NULL)
		FAIL("get_hugepage_region() for %d hugepages", num_hugepages);

	memset(p, 1, TESTLEN);

	err = test_unaligned_addr_huge(p + (num_hugepages - 1) * hpage_size);
	if (err != 1)
		FAIL("Returned page is not hugepage");

	free_and_confirm_region_free(p, __LINE__);
	err = test_unaligned_addr_huge(p);
	if (err == 1)
		FAIL("hugepage was not correctly freed");
}

void test_GHR_FALLBACK(void)
{
	int err;
	long rsvd_hugepages = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	long num_hugepages = get_huge_page_counter(hpage_size, HUGEPAGES_TOTAL)
		- rsvd_hugepages;

	/* We must disable overcommitted huge pages to test this */
	oc_hugepages = get_huge_page_counter(hpage_size, HUGEPAGES_OC);
	set_nr_overcommit_hugepages(hpage_size, 0);

	/* We should be able to allocate the whole pool */
	void *p = get_hugepage_region(TESTLEN, GHR_DEFAULT);
	if (p == NULL)
		FAIL("test_GHR_FALLBACK(GHR_DEFAULT) failed for %ld hugepages",
			num_hugepages);
	memset(p, 1, TESTLEN);
	err = test_unaligned_addr_huge(p + (num_hugepages - 1) * hpage_size);
	if (err != 1)
		FAIL("Returned page is not hugepage");
	free_and_confirm_region_free(p, __LINE__);

	/* We should fail allocating too much */
	num_hugepages++;
	p = get_hugepage_region(TESTLEN, GHR_STRICT);
	if (p != NULL)
		FAIL("test_GHR_FALLBACK() for %ld expected fail, got success", num_hugepages);

	/* GHR_FALLBACK should succeed by allocating base pages */
	p = get_hugepage_region(TESTLEN, GHR_FALLBACK);
	if (p == NULL)
		FAIL("test_GHR_FALLBACK(GHR_FALLBACK) failed for %ld hugepages",
			num_hugepages);
	memset(p, 1, TESTLEN);
	err = test_unaligned_addr_huge(p + (num_hugepages - 1) * hpage_size);
	if (err == 1)
		FAIL("Returned page is not a base page");

	/*
	 * We allocate a second fallback region to see can they be told apart
	 * on free. Merging VMAs would cause problems
	 */
	void *pb = get_hugepage_region(TESTLEN, GHR_FALLBACK);
	if (pb == NULL)
		FAIL("test_GHR_FALLBACK(GHR_FALLBACK) x2 failed for %ld hugepages",
			num_hugepages);
	memset(pb, 1, TESTLEN);

	free_and_confirm_region_free(pb, __LINE__);
	free_and_confirm_region_free(p, __LINE__);
}

int main(int argc, char *argv[])
{
	test_init(argc, argv);
	hpage_size = gethugepagesize();
	check_free_huge_pages(4);
	test_GHR_STRICT(1);
	test_GHR_STRICT(4);
	test_GHR_FALLBACK();

	PASS();
}

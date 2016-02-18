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

/*
 * This file should only contain definitions of functions, data types, and
 * constants which are part of the internal private utilities interfaces.
 * These are exposed only to utilities and tests within the source, this is
 * not a public interface nor part of the libhugetlfs API.
 *
 * All functions declared external here must be externalised using a define
 * of the following form:
 *
 * 	#define foo __pu_foo
 */

#ifndef _LIBHUGETLBFS_PRIVUTILS_H
#define _LIBHUGETLBFS_PRIVUTILS_H

/* Hugetlb pool counter operations */
/* Keys for reading hugetlb pool counters */
enum {		 	/* The number of pages of a given size that ... */
	HUGEPAGES_TOTAL, 	/* are allocated to the pool */
	HUGEPAGES_TOTAL_MEMPOL,	/* are allocated following the NUMA mempolicy */
	HUGEPAGES_FREE,  	/* are not in use */
	HUGEPAGES_RSVD,  	/* are reserved for possible future use */
	HUGEPAGES_SURP,  	/* are allocated to the pool on demand */
	HUGEPAGES_OC,    	/* can be allocated on demand - maximum */
	HUGEPAGES_MAX_COUNTERS,
};
#define get_huge_page_counter __pu_get_huge_page_counter
long get_huge_page_counter(long pagesize, unsigned int counter);
#define set_huge_page_counter __pu_set_huge_page_counter
int set_huge_page_counter(long pagesize, unsigned int counter,
							unsigned long val);
#define set_nr_hugepages __pu_set_nr_hugepages
int set_nr_hugepages(long pagesize, unsigned long val);
#define set_nr_overcommit_hugepages __pu_set_nr_overcommit_hugepages
int set_nr_overcommit_hugepages(long pagesize, unsigned long val);

#define kernel_has_hugepages __pu_kernel_has_hugepages
int kernel_has_hugepages(void);

#define kernel_has_overcommit __pu_kernel_has_overcommit
int kernel_has_overcommit(void);

#define read_meminfo __pu_read_meminfo
long read_meminfo(const char *tag);

#define kernel_default_hugepage_size __pu_kernel_default_hugepage_size
long kernel_default_hugepage_size(void);

#define read_nr_overcommit __pu_read_nr_overcommit
long read_nr_overcommit(long page_size);

#define restore_overcommit_pages __pu_restore_overcommit_pages
void restore_overcommit_pages(long page_size, long oc_pool);

/* Kernel feature testing */
/* This enum defines the bits in a feature bitmask */
enum {
	/* Reservations are created for private mappings */
	HUGETLB_FEATURE_PRIVATE_RESV,

	/* Whether use of MAP_NORESERVE is safe or can result in OOM */
	HUGETLB_FEATURE_SAFE_NORESERVE,

	/* If the kernel has the ability to mmap(MAP_HUGETLB)*/
	HUGETLB_FEATURE_MAP_HUGETLB,

	HUGETLB_FEATURE_NR,
};
#define hugetlbfs_test_feature __pu_hugetlbfs_test_feature
int hugetlbfs_test_feature(int feature_code);

#define test_compare_kver __pu_test_compare_kver
int test_compare_kver(const char *a, const char *b);

#endif /* _LIBHUGETLBFS_PRIVUTILS_H */

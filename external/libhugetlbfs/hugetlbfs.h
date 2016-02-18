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
 * constants which are part of the published libhugetlfs API.  Functions
 * exported here must also be listed in version.lds.
 */

#ifndef _HUGETLBFS_H
#define _HUGETLBFS_H

#define HUGETLBFS_MAGIC	0x958458f6

long gethugepagesize(void);
int gethugepagesizes(long pagesizes[], int n_elem);
int getpagesizes(long pagesizes[], int n_elem);
int hugetlbfs_test_path(const char *mount);
const char *hugetlbfs_find_path(void);
const char *hugetlbfs_find_path_for_size(long page_size);
int hugetlbfs_unlinked_fd(void);
int hugetlbfs_unlinked_fd_for_size(long page_size);

#define PF_LINUX_HUGETLB	0x100000

/*
 * Direct hugepage allocation flags and types
 *
 * GHP_DEFAULT - Use the default hugepage size to back the region
 */
typedef unsigned long ghp_t;
#define GHP_DEFAULT	((ghp_t)0x01UL)
#define GHP_MASK	(GHP_DEFAULT)

/* Direct alloc functions for hugepages */
void *get_huge_pages(size_t len, ghp_t flags);
void free_huge_pages(void *ptr);

/*
 * Region alloc flags and types
 *
 * GHR_DEFAULT  - Use a combination of flags deemed to be a sensible default
 * 		  by the current implementation of the library
 * GHR_FALLBACK - Use the default hugepage size if possible but fallback to
 * 		  smaller pages if necessary
 * GHR_STRICT   - Use hugepages of some size or return NULL
 * GHP_COLOR    - Use bytes wasted due to alignment to offset the buffer
 *		  by a random cache line. This gives better average
 *		  performance with many buffers
 */
typedef unsigned long ghr_t;
#define GHR_STRICT	((ghr_t)0x10000000U)
#define GHR_FALLBACK	((ghr_t)0x20000000U)
#define GHR_COLOR	((ghr_t)0x40000000U)
#define GHR_DEFAULT	(GHR_FALLBACK|GHR_COLOR)

#define GHR_MASK	(GHR_FALLBACK|GHR_STRICT|GHR_COLOR)

/* Allocation functions for regions backed by hugepages */
void *get_hugepage_region(size_t len, ghr_t flags);
void free_hugepage_region(void *ptr);

#endif /* _HUGETLBFS_H */

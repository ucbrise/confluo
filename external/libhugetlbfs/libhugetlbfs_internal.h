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
 * constants which are used internally within the libhugetlbfs library.
 *
 * All external functions declared here are library static and must be
 * internalised using a define of the following form:
 *
 * 	#define foo __lh_foo
 */

#ifndef _LIBHUGETLBFS_INTERNAL_H
#define _LIBHUGETLBFS_INTERNAL_H

#include <elf.h>
#include <link.h>
#include <limits.h>
#include <stdbool.h>

#ifndef __LIBHUGETLBFS__
#error This header should not be included by library users.
#endif /* __LIBHUGETLBFS__ */

#include "libhugetlbfs_privutils.h"
#include "libhugetlbfs_testprobes.h"

#define stringify_1(x)	#x
#define stringify(x)	stringify_1(x)

#define ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_UP(x,a)	ALIGN(x,a)
#define ALIGN_DOWN(x,a) ((x) & ~((a) - 1))

#if defined(__powerpc64__) || \
	(defined(__powerpc__) && !defined(PPC_NO_SEGMENTS))
#define SLICE_LOW_SHIFT		28
#define SLICE_HIGH_SHIFT	40
#elif defined(__ia64__)
#define SLICE_HIGH_SHIFT	63
#endif

struct libhugeopts_t {
	int		sharing;
	bool		min_copy;
	bool		shrink_ok;
	bool		shm_enabled;
	bool		no_reserve;
	bool		map_hugetlb;
	bool		thp_morecore;
	unsigned long	force_elfmap;
	char		*ld_preload;
	char		*elfmap;
	char		*share_path;
	char 		*features;
	char		*path;
	char		*def_page_size;
	char		*morecore;
	char		*heapbase;
};

/*
 * When adding a library local variable externalise the symbol as
 * normal, plus add a #define of the form below.  This define effectively
 * renames the routine into the local namespace __lh_* which is forced
 * local in the linker script version.lds.  Some routines may need to be
 * exported in the utilities library these are marked __pu_* which marks
 * them for export in libhugetlbfs_privutils; their definitions should
 * appear in libhugetlbfs_privutils.h rather than here.
 */
#define __hugetlbfs_verbose __lh___hugetlbfs_verbose
extern int __hugetlbfs_verbose;
#define __hugetlbfs_debug __lh___hugetlbfs_debug
extern bool __hugetlbfs_debug;
#define __hugetlbfs_prefault __lh___hugetlbfs_prefault
extern bool __hugetlbfs_prefault;
#define hugetlbfs_setup_env __lh_hugetlbfs_setup_env
extern void hugetlbfs_setup_env();
#define hugetlbfs_setup_elflink __lh_hugetlbfs_setup_elflink
extern void hugetlbfs_setup_elflink();
#define hugetlbfs_setup_morecore __lh_hugetlbfs_setup_morecore
extern void hugetlbfs_setup_morecore();
#define hugetlbfs_setup_debug __lh_hugetlbfs_setup_debug
extern void hugetlbfs_setup_debug();
#define setup_mounts __lh_setup_mounts
extern void setup_mounts();
#define setup_features __lh_setup_features
extern void setup_features();
#define hugetlbfs_check_priv_resv __lh_hugetlbfs_check_priv_resv
extern void hugetlbfs_check_priv_resv();
#define hugetlbfs_check_safe_noreserve __lh_hugetlbfs_check_safe_noreserve
extern void hugetlbfs_check_safe_noreserve();
#define hugetlbfs_check_map_hugetlb __lh_hugetblfs_check_map_hugetlb
extern void hugetlbfs_check_map_hugetlb();
#define __hugetlbfs_hostname __lh___hugetlbfs_hostname
extern char __hugetlbfs_hostname[];
#define hugetlbfs_prefault __lh_hugetlbfs_prefault
extern int hugetlbfs_prefault(void *addr, size_t length);
#define parse_page_size __lh_parse_page_size
extern long parse_page_size(const char *str);
#define probe_default_hpage_size __lh__probe_default_hpage_size
extern void probe_default_hpage_size(void);
#define debug_show_page_sizes __lh__debug_show_page_sizes
extern void debug_show_page_sizes(void);
#define hugetlbfs_setup_kernel_page_size __lh__hugetlbfs_setup_kernel_page_size
extern void hugetlbfs_setup_kernel_page_size(void);
#define __hugetlb_opts __lh__hugetlb_opts
extern struct libhugeopts_t __hugetlb_opts;

#ifndef REPORT_UTIL
#define REPORT_UTIL "libhugetlbfs"
#endif

#define VERBOSE_ERROR	1
#define VERBOSE_WARNING	2
#define VERBOSE_INFO	3
#define VERBOSE_DEBUG	4

#ifndef REPORT
#define REPORT(level, prefix, format, ...) \
	do { \
		if (__hugetlbfs_verbose >= level) { \
			fprintf(stderr, REPORT_UTIL); \
			if (__hugetlbfs_verbose >= VERBOSE_DEBUG) \
				fprintf(stderr, " [%s:%d]", \
					__hugetlbfs_hostname, getpid()); \
			fprintf(stderr, ": " prefix ": " format, \
				##__VA_ARGS__); \
			fflush(stderr); \
		} \
	} while (0)

#define REPORT_CONT(level, prefix, ...) \
	do { \
		if (__hugetlbfs_verbose >= level) { \
			fprintf(stderr, ##__VA_ARGS__); \
			fflush(stderr); \
		} \
	} while (0)
#endif

#include "libhugetlbfs_debug.h"

#if defined(__powerpc64__) && !defined(__LP64__)
/* Older binutils fail to provide this symbol */
#define __LP64__
#endif

/* Multiple huge page size support */
struct hpage_size {
	unsigned long pagesize;
	char mount[PATH_MAX+1];
};

struct hpage_pool {
	unsigned long pagesize;
	unsigned long minimum;
	unsigned long maximum;
	unsigned long size;
	int is_default;
};

#define size_to_smaller_unit __lh_size_to_smaller_unit
extern unsigned long long size_to_smaller_unit(unsigned long long size);

#define file_read_ulong __lh_file_read_ulong
extern long file_read_ulong(char *file, const char *tag);
#define file_write_ulong __lh_file_write_ulong
extern int file_write_ulong(char *file, unsigned long val);

#define hpool_sizes __lh_hpool_sizes
extern int hpool_sizes(struct hpage_pool *, int);
#define get_pool_size __lh_get_pool_size
extern int get_pool_size(long, struct hpage_pool *);

/* Arch-specific callbacks */
extern int direct_syscall(int sysnum, ...);
extern ElfW(Word) plt_extrasz(ElfW(Dyn) *dyntab);

#define MEMINFO "/proc/meminfo"
#define PROC_HUGEPAGES_DIR "/proc/sys/vm/"
#define SYSFS_HUGEPAGES_DIR "/sys/kernel/mm/hugepages/"

#define hugetlbfs_test_pagesize __lh_hugetlbfs_test_pagesize
long hugetlbfs_test_pagesize(const char *mount);

/* Diagnoses/debugging only functions */
#define dump_proc_pid_maps __lh_dump_proc_pid_maps
long dump_proc_pid_maps(void);

#define plt_extrasz __lh_plt_extrasz
ElfW(Word) plt_extrasz(ElfW(Dyn) *dyntab);

#endif /* _LIBHUGETLBFS_INTERNAL_H */

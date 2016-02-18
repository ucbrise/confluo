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

#define _LARGEFILE64_SOURCE /* Need this for statfs64 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <features.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <dirent.h>

#include "libhugetlbfs_internal.h"
#include "hugetlbfs.h"

struct libhugeopts_t __hugetlb_opts;

static int hugepagesize_errno; /* = 0 */

#define MAX_HPAGE_SIZES 10
static struct hpage_size hpage_sizes[MAX_HPAGE_SIZES];
static int nr_hpage_sizes;
static int hpage_sizes_default_idx = -1;

static long default_size;

/********************************************************************/
/* Internal functions                                               */
/********************************************************************/

/*
 * Lookup the kernel default page size.
 */
long kernel_default_hugepage_size()
{
	if (default_size == 0) {
		default_size = file_read_ulong(MEMINFO, "Hugepagesize:");
		default_size = size_to_smaller_unit(default_size); /* kB to B */	}
	return default_size;
}
void kernel_default_hugepage_size_reset(void)
{
	default_size = 0;
}

#define BUF_SZ 256
#define MEMINFO_SIZE	2048

/*
 * Convert a quantity in a given unit to the next smallest unit by
 * multiplying the quantity by 1024 (eg. convert 1MB to 1024kB).
 * If the conversion would overflow the variable, return ULONGLONG_MAX to
 * signify the error.
 */
unsigned long long size_to_smaller_unit(unsigned long long size)
{
	if (size * 1024 < size)
		return -1;
	else
		return size * 1024;
}

/*
 * Convert a page size string with an optional unit suffix into a page size
 * in bytes.
 *
 * On error, -1 is returned and errno is set appropriately:
 * 	EINVAL		- str could not be parsed or was not greater than zero
 *	EOVERFLOW	- Overflow when converting from the specified units
 */
long parse_page_size(const char *str)
{
	char *pos;
	long size;

	errno = 0;
	size = strtol(str, &pos, 0);
	/* Catch strtoul errors and sizes that overflow the native word size */
	if (errno || str == pos || size <= 0) {
		if (errno == ERANGE)
			errno = EOVERFLOW;
		else
			errno = EINVAL;
		return -1;
	}

	switch (*pos) {
	case 'G':
	case 'g':
		size = size_to_smaller_unit(size);
	case 'M':
	case 'm':
		size = size_to_smaller_unit(size);
	case 'K':
	case 'k':
		size = size_to_smaller_unit(size);
	}

	if (size < 0)
		errno = EOVERFLOW;
	return size;
}

struct hugetlb_pool_counter_info_t {
	char *meminfo_key;
	char *sysfs_file;
};

static struct hugetlb_pool_counter_info_t hugetlb_counter_info[] = {
	[HUGEPAGES_TOTAL] = {
		.meminfo_key	= "HugePages_Total:",
		.sysfs_file	= "nr_hugepages",
	},
	[HUGEPAGES_TOTAL_MEMPOL] = {
		.meminfo_key	= "HugePages_Total:",
		.sysfs_file	= "nr_hugepages_mempolicy",
	},
	[HUGEPAGES_FREE] = {
		.meminfo_key	= "HugePages_Free:",
		.sysfs_file	= "free_hugepages",
	},
	[HUGEPAGES_RSVD] = {
		.meminfo_key	= "HugePages_Rsvd:",
		.sysfs_file	= "resv_hugepages",
	},
	[HUGEPAGES_SURP] = {
		.meminfo_key	= "HugePages_Surp:",
		.sysfs_file	= "surplus_hugepages",
	},
	[HUGEPAGES_OC] = {
		.meminfo_key	= NULL,
		.sysfs_file	= "nr_overcommit_hugepages"
	},
};

/*
 * Read numeric data from raw and tagged kernel status files.  Used to read
 * /proc and /sys data (without a tag) and from /proc/meminfo (with a tag).
 */
long file_read_ulong(char *file, const char *tag)
{
	int fd;
	char buf[MEMINFO_SIZE];
	int len, readerr;
	char *p, *q;
	long val;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		ERROR("Couldn't open %s: %s\n", file, strerror(errno));
		return -1;
	}

	len = read(fd, buf, sizeof(buf));
	readerr = errno;
	close(fd);
	if (len < 0) {
		ERROR("Error reading %s: %s\n", file, strerror(readerr));
		return -1;
	}
	if (len == sizeof(buf)) {
		ERROR("%s is too large\n", file);
		return -1;
	}
	buf[len] = '\0';

	/* Search for a tag if provided */
	if (tag) {
		p = strstr(buf, tag);
		if (!p)
			return -1; /* looks like the line we want isn't there */
		p += strlen(tag);
	} else
		p = buf;

	val = strtol(p, &q, 0);
	if (! isspace(*q)) {
		ERROR("Couldn't parse %s value\n", file);
		return -1;
	}

	return val;
}

int file_write_ulong(char *file, unsigned long val)
{
	FILE *f;
	int ret;

	f = fopen(file, "w");
	if (!f) {
		ERROR("Couldn't open %s: %s\n", file, strerror(errno));
		return -1;
	}

	ret = fprintf(f, "%lu", val);
	fclose(f);
	return ret > 0 ? 0 : -1;
}


/*
 * Return the name of this executable, using buf as temporary space.
 */
#define MAX_EXE 4096
static char *get_exe_name(char *buf, int size)
{
	char *p;
	int fd;
	ssize_t nread;

	buf[0] = 0;
	fd = open("/proc/self/cmdline", O_RDONLY);
	if (fd < 0) {
		WARNING("Unable to open cmdline, no exe name\n");
		return buf;
	}
	nread = read(fd, buf, size-1);
	close(fd);

	if (nread < 0) {
		WARNING("Error %d reading cmdline, no exe name\n", errno);
		return buf;
	}
	if (nread == 0) {
		WARNING("Read zero bytes from cmdline, no exe name\n");
		return buf;
	}

	buf[nread] = 0; /* make sure we're null terminated */
	/*
	 * Take advantage of cmdline being a series of null-terminated
	 * strings.  The first string is the path to the executable in
	 * the form:
	 *
	 *      /path/to/exe
	 *
	 * The exe name starts one character after the last '/'.
	 */
	p = strrchr(buf, '/');
	if (!p)
		return buf;
	return p + 1;           /* skip over "/" */
}


/*
 * Reads the contents of hugetlb environment variables and save their
 * values for later use.
 */
void hugetlbfs_setup_env()
{
	char *env;

	__hugetlb_opts.min_copy = true;

	env = getenv("HUGETLB_VERBOSE");
	if (env)
		__hugetlbfs_verbose = atoi(env);

	env = getenv("HUGETLB_DEBUG");
	if (env) {
		__hugetlbfs_debug = true;
		__hugetlbfs_verbose = VERBOSE_DEBUG;
	}

	env = getenv("HUGETLB_RESTRICT_EXE");
	if (env) {
		char *p, *tok, *exe, buf[MAX_EXE+1], restriction[MAX_EXE];
		int found = 0;

		exe = get_exe_name(buf, sizeof buf);
		DEBUG("Found HUGETLB_RESTRICT_EXE, this exe is \"%s\"\n", exe);
		strncpy(restriction, env, sizeof restriction);
		restriction[sizeof(restriction)-1] = 0;
		for (p = restriction; (tok = strtok(p, ":")) != NULL; p = NULL) {
			DEBUG("  ...check exe match for \"%s\"\n",  tok);
			if (strcmp(tok, exe) == 0) {
				found = 1;
				DEBUG("exe match - libhugetlbfs is active for this exe\n");
				break;
			}
		}
		if (!found) {
			DEBUG("No exe match - libhugetlbfs is inactive for this exe\n");
			return;
		}
	}

	env = getenv("HUGETLB_NO_PREFAULT");
	if (env)
		__hugetlbfs_prefault = false;

	__hugetlb_opts.share_path = getenv("HUGETLB_SHARE_PATH");
	__hugetlb_opts.elfmap = getenv("HUGETLB_ELFMAP");
	__hugetlb_opts.ld_preload = getenv("LD_PRELOAD");
	__hugetlb_opts.def_page_size = getenv("HUGETLB_DEFAULT_PAGE_SIZE");
	__hugetlb_opts.path = getenv("HUGETLB_PATH");
	__hugetlb_opts.features = getenv("HUGETLB_FEATURES");
	__hugetlb_opts.morecore = getenv("HUGETLB_MORECORE");
	__hugetlb_opts.heapbase = getenv("HUGETLB_MORECORE_HEAPBASE");

	if (__hugetlb_opts.morecore)
		__hugetlb_opts.thp_morecore =
			(strcasecmp(__hugetlb_opts.morecore, "thp") == 0);

	if (__hugetlb_opts.thp_morecore && __hugetlb_opts.heapbase) {
		DEBUG("Heapbase specified with THP for morecore, ignoring heapbase\n");
		__hugetlb_opts.heapbase = NULL;
	}

	env = getenv("HUGETLB_FORCE_ELFMAP");
	if (env && (strcasecmp(env, "yes") == 0))
		__hugetlb_opts.force_elfmap = 1;

	env = getenv("HUGETLB_MINIMAL_COPY");
	if (__hugetlb_opts.min_copy && env && (strcasecmp(env, "no") == 0)) {
		INFO("HUGETLB_MINIMAL_COPY=%s, disabling filesz copy "
			"optimization\n", env);
		__hugetlb_opts.min_copy = false;
	}

	env = getenv("HUGETLB_SHARE");
	if (env)
		__hugetlb_opts.sharing = atoi(env);

	/*
	 * We have been seeing some unexpected behavior from malloc when
	 * heap shrinking is enabled, so heap shrinking is disabled by
	 * default.
	 *
	 * If malloc has been called successfully before setup_morecore,
	 * glibc will notice a gap between the previous top-of-heap and
	 * the new top-of-heap when it calls hugetlbfs_morecore.  It treats
	 * this as a "foreign sbrk."  Unfortunately, the "foreign sbrk"
	 * handling code will then immediately try to free the memory
	 * allocated by hugetlbfs_morecore!
	 *
	 * This behavior has been reported to the ptmalloc2 maintainer,
	 * along with a patch to correct the behavior.
	 */
	env = getenv("HUGETLB_MORECORE_SHRINK");
	if (env && strcasecmp(env, "yes") == 0)
		__hugetlb_opts.shrink_ok = true;

	/* Determine if shmget() calls should be overridden */
	env = getenv("HUGETLB_SHM");
	if (env && !strcasecmp(env, "yes"))
		__hugetlb_opts.shm_enabled = true;

	/* Determine if all reservations should be avoided */
	env = getenv("HUGETLB_NO_RESERVE");
	if (env && !strcasecmp(env, "yes"))
		__hugetlb_opts.no_reserve = true;
}

void hugetlbfs_setup_kernel_page_size()
{
	long page_size = kernel_default_hugepage_size();

	if (page_size <= 0) {
		WARNING("Unable to find default kernel huge page size\n");
		return;
	}

	INFO("Found pagesize %ld kB\n", page_size / 1024);
	hpage_sizes[0].pagesize = page_size;

	nr_hpage_sizes = 1;
}

void hugetlbfs_check_priv_resv()
{
	/*
	 * If the kernel supports MAP_PRIVATE reservations, we can skip
	 * prefaulting the huge pages we allocate since the kernel
	 * guarantees them.  This can help NUMA performance quite a bit.
	 */
	if (hugetlbfs_test_feature(HUGETLB_FEATURE_PRIVATE_RESV) > 0) {
		INFO("Kernel has MAP_PRIVATE reservations.  Disabling "
			"heap prefaulting.\n");
		__hugetlbfs_prefault = false;
	}
}

void hugetlbfs_check_safe_noreserve()
{
	/*
	 * Some kernels will trigger an OOM if MAP_NORESERVE is used and
	 * a huge page allocation fails. This is unfortunate so limit
	 * the user of NORESERVE where necessary
	 */
	if (__hugetlb_opts.no_reserve &&
		hugetlbfs_test_feature(HUGETLB_FEATURE_SAFE_NORESERVE) <= 0) {
		INFO("Kernel is not safe for MAP_NORESERVE. Forcing "
			"use of reservations.\n");
		__hugetlb_opts.no_reserve = false;
	}
}

void hugetlbfs_check_map_hugetlb()
{
/*
 * FIXME: MAP_HUGETLB has not been picked up by glibc so even though the
 * kernel may support it, without the userspace mmap flag it cannot be
 * used.  This ifdef should be removed when the MAP_HUGETLB flag makes it
 * into glibc.
 */
#ifdef MAP_HUGETLB
	/*
	 * Kernels after 2.6.32 support mmaping pseudo-anonymous regions
	 * backed by huge pages, use this feature for huge pages we
	 * don't intend to share.
	 */
	if (hugetlbfs_test_feature(HUGETLB_FEATURE_MAP_HUGETLB) > 0) {
		INFO("Kernel supports MAP_HUGETLB\n");
		__hugetlb_opts.map_hugetlb = true;
	}
#endif
}

/*
 * Pool counters are typically exposed in sysfs in modern kernels, the
 * counters for the default page size are exposed in procfs in all kernels
 * supporting hugepages.  Given a specific counter (e.g. HUGEPAGES_RSVD)
 * and a page size return both a filename and an optional tag to locate
 * and extract this counter.
 */
static int select_pool_counter(unsigned int counter, unsigned long pagesize,
				char *filename, char **key)
{
	long default_size;
	char *meminfo_key;
	char *sysfs_file;

	if (counter >= HUGEPAGES_MAX_COUNTERS) {
		ERROR("Invalid counter specified\n");
		return -1;
	}

	meminfo_key = hugetlb_counter_info[counter].meminfo_key;
	sysfs_file = hugetlb_counter_info[counter].sysfs_file;
	if (key)
		*key = NULL;

	/*
	 * Get the meminfo page size.
	 * This could be made more efficient if utility functions were shared
	 * between libhugetlbfs and the test suite.  For now we will just
	 * read /proc/meminfo.
	 */
	default_size = kernel_default_hugepage_size();
	if (default_size < 0) {
		ERROR("Cannot determine the default page size\n");
		return -1;
	}

	/* If the user is dealing in the default page size, we can use /proc */
	if (pagesize == default_size) {
		if (meminfo_key && key) {
			strcpy(filename, MEMINFO);
			*key = meminfo_key;
		} else
			sprintf(filename, PROC_HUGEPAGES_DIR "%s", sysfs_file);
	} else /* Use the sysfs interface */
		sprintf(filename, SYSFS_HUGEPAGES_DIR "hugepages-%lukB/%s",
			pagesize / 1024, sysfs_file);
	return 0;
}

static int hpage_size_to_index(unsigned long size)
{
	int i;

	for (i = 0; i < nr_hpage_sizes; i++)
		if (hpage_sizes[i].pagesize == size)
			return i;
	return -1;
}

void probe_default_hpage_size(void)
{
	long size;
	int index;
	int default_overrided;

	if (nr_hpage_sizes == 0) {
		INFO("No configured huge page sizes\n");
		hpage_sizes_default_idx = -1;
		return;
	}

	/*
	 * Check if the user specified a default size, otherwise use the
	 * system default size as reported by /proc/meminfo.
	 */
	default_overrided = (__hugetlb_opts.def_page_size &&
				strlen(__hugetlb_opts.def_page_size) > 0);
	if (default_overrided)
		size = parse_page_size(__hugetlb_opts.def_page_size);
	else {
		size = kernel_default_hugepage_size();
	}

	if (size >= 0) {
		index = hpage_size_to_index(size);
		if (index >= 0)
			hpage_sizes_default_idx = index;
		else {
			/*
			 * If the user specified HUGETLB_DEFAULT_PAGE_SIZE,
			 * then this situation will alter semantics and they
			 * should receive a WARNING.  Otherwise, this detail
			 * is purely informational in nature.
			 */
			char msg[] = "No mount point found for default huge " \
				"page size. Using first available mount "
				"point.\n";
			if (default_overrided)
				WARNING("%s", msg);
			else
				INFO("%s", msg);
			hpage_sizes_default_idx = 0;
		}
	} else {
		ERROR("Unable to determine default huge page size\n");
		hpage_sizes_default_idx = -1;
	}
}

static void add_hugetlbfs_mount(char *path, int user_mount)
{
	int idx;
	long size;

	if (strlen(path) > PATH_MAX)
		return;

	if (!hugetlbfs_test_path(path)) {
		WARNING("%s is not a hugetlbfs mount point, ignoring\n", path);
		return;
	}

	size = hugetlbfs_test_pagesize(path);
	if (size < 0) {
		WARNING("Unable to detect page size for path %s\n", path);
		return;
	}

	idx = hpage_size_to_index(size);
	if (idx < 0) {
		if (nr_hpage_sizes >= MAX_HPAGE_SIZES) {
			WARNING("Maximum number of huge page sizes exceeded, "
				"ignoring %lukB page size\n", size);
			return;
		}

		idx = nr_hpage_sizes;
		hpage_sizes[nr_hpage_sizes++].pagesize = size;
	}

	if (strlen(hpage_sizes[idx].mount)) {
		if (user_mount)
			WARNING("Mount point already defined for size %li, "
				"ignoring %s\n", size, path);
		return;
	}

	strcpy(hpage_sizes[idx].mount, path);
}

void debug_show_page_sizes(void)
{
	int i;

	INFO("Detected page sizes:\n");
	for (i = 0; i < nr_hpage_sizes; i++)
		INFO("   Size: %li kB %s  Mount: %s\n",
			hpage_sizes[i].pagesize / 1024,
			i == hpage_sizes_default_idx ? "(default)" : "",
			hpage_sizes[i].mount);
}

#define LINE_MAXLEN	2048
static void find_mounts(void)
{
	int fd;
	char path[PATH_MAX+1];
	char line[LINE_MAXLEN + 1];
	char *eol;
	char *match;
	char *end;
	int bytes;
	off_t offset;

	fd = open("/proc/mounts", O_RDONLY);
	if (fd < 0) {
		fd = open("/etc/mtab", O_RDONLY);
		if (fd < 0) {
			ERROR("Couldn't open /proc/mounts or /etc/mtab (%s)\n",
				strerror(errno));
			return;
		}
	}

	while ((bytes = read(fd, line, LINE_MAXLEN)) > 0) {
		line[LINE_MAXLEN] = '\0';
		eol = strchr(line, '\n');
		if (!eol) {
			ERROR("Line too long when parsing mounts\n");
			break;
		}

		/*
		 * Truncate the string to just one line and reset the file
		 * to begin reading at the start of the next line.
		 */
		*eol = '\0';
		offset = bytes - (eol + 1 - line);
		lseek(fd, -offset, SEEK_CUR);

		/* Match only hugetlbfs filesystems. */
		match = strstr(line, " hugetlbfs ");
		if (match) {
			match = strchr(line, '/');
			if (!match)
				continue;
			end = strchr(match, ' ');
			if (!end)
				continue;

			strncpy(path, match, end - match);
			path[end - match] = '\0';
			if ((hugetlbfs_test_path(path) == 1) &&
			    !(access(path, R_OK | W_OK | X_OK)))
				add_hugetlbfs_mount(path, 0);
		}
	}
	close(fd);
}

void setup_mounts(void)
{
	int do_scan = 1;

	/* If HUGETLB_PATH is set, only add mounts specified there */
	while (__hugetlb_opts.path) {
		char path[PATH_MAX + 1];
		char *next = strchrnul(__hugetlb_opts.path, ':');

		do_scan = 0;
		if (next - __hugetlb_opts.path > PATH_MAX) {
			ERROR("Path too long in HUGETLB_PATH -- "
				"ignoring environment\n");
			break;
		}

		strncpy(path, __hugetlb_opts.path, next - __hugetlb_opts.path);
		path[next - __hugetlb_opts.path] = '\0';
		add_hugetlbfs_mount(path, 1);

		/* skip the ':' token */
		__hugetlb_opts.path = *next == '\0' ? NULL : next + 1;
	}

	/* Then probe all mounted filesystems */
	if (do_scan)
		find_mounts();
}

int get_pool_size(long size, struct hpage_pool *pool)
{
	long nr_over = 0;
	long nr_used = 0;
	long nr_surp = 0;
	long nr_resv = 0;
	long nr_static = 0;

	long it_used = -1;
	long it_surp = -1;
	long it_resv = -1;

	/*
	 * Pick up those values which are basically stable with respect to
	 * the admin; ie. only changed by them.
	 *
	 * nr_over may be negative if this kernel does not support overcommit
	 * in that case we will consider it always 0 and max will track min
	 * always.
	 */
	nr_over = get_huge_page_counter(size, HUGEPAGES_OC);
	if (nr_over < 0)
		nr_over = 0;

	/* Sample the volatile values until they are stable. */
	while (nr_used != it_used || nr_surp != it_surp || nr_resv != it_resv) {
		nr_used = it_used;
		nr_surp = it_surp;
		nr_resv = it_resv;

		it_used = get_huge_page_counter(size, HUGEPAGES_TOTAL);
		it_surp = get_huge_page_counter(size, HUGEPAGES_SURP);
		it_resv = get_huge_page_counter(size, HUGEPAGES_RSVD);
	}
	if (nr_surp < 0)
		nr_surp = 0;
	if (nr_resv < 0)
		nr_resv = 0;

	nr_static = nr_used - nr_surp;

	if (nr_static >= 0) {
		DEBUG("pagesize<%ld> min<%ld> max<%ld> "
			"in-use<%ld>\n",
			size, nr_static, nr_static + nr_over,
			nr_used);
		pool->pagesize = size;
		pool->minimum = nr_static;
		pool->maximum = nr_static + nr_over;
		pool->size = nr_used;
		pool->is_default = 0;

		return 1;
	}

	return 0;
}

int hpool_sizes(struct hpage_pool *pools, int pcnt)
{
	long default_size;
	int which = 0;
	DIR *dir;
	struct dirent *entry;

	default_size = kernel_default_hugepage_size();
	if (default_size >= 0 && which < pcnt)
		if (get_pool_size(default_size, &pools[which])) {
			pools[which].is_default = 1;
			which++;
		}

	dir = opendir(SYSFS_HUGEPAGES_DIR);
	if (dir) {
		while ((entry = readdir(dir))) {
			char *name = entry->d_name;
			long size;

			DEBUG("parsing<%s>\n", name);
			if (strncmp(name, "hugepages-", 10) != 0)
				continue;
			name += 10;

			size = size_to_smaller_unit(atol(name));
			if (size < 0 || size == default_size)
				continue;

			if (get_pool_size(size, &pools[which]))
				which++;
		}
		closedir(dir);
	}

	return (which < pcnt) ? which : -1;
}

/*
 * If we have a default page size then we support hugepages.
 */
int kernel_has_hugepages(void)
{
	long default_size = kernel_default_hugepage_size();
	if (default_size < 0)
		return 0;

	return 1;
}

/*
 * If we can find the default page size, and if we can find an overcommit
 * control for it then the kernel must support overcommit.
 */
int kernel_has_overcommit(void)
{
	long default_size = kernel_default_hugepage_size();
	if (default_size < 0)
		return 0;

	if (get_huge_page_counter(default_size, HUGEPAGES_OC) < 0)
		return 0;

	return 1;
}

/********************************************************************/
/* Library user visible functions                                   */
/********************************************************************/

/*
 * NOTE: This function uses data that is initialized by
 * setup_mounts() which is called during libhugetlbfs initialization.
 *
 * returns:
 *   on success, size of a huge page in number of bytes
 *   on failure, -1
 *	errno set to ENOSYS if huge pages are not supported
 *	errno set to EOVERFLOW if huge page size would overflow return type
 */
long gethugepagesize(void)
{
	long hpage_size;

	/* Are huge pages available and have they been initialized? */
	if (hpage_sizes_default_idx == -1) {
		errno = hugepagesize_errno = ENOSYS;
		return -1;
	}

	errno = 0;
	hpage_size = hpage_sizes[hpage_sizes_default_idx].pagesize;
	return hpage_size;
}

int gethugepagesizes(long pagesizes[], int n_elem)
{
	long default_size;
	DIR *sysfs;
	struct dirent *ent;
	int nr_sizes = 0;

	if (n_elem < 0) {
		errno = EINVAL;
		return -1;
	}

	if (n_elem > 0 && pagesizes == NULL) {
		errno = EINVAL;
		return -1;
	}

	errno = 0;

	/* Get the system default size. */
	default_size = kernel_default_hugepage_size();
	if (default_size < 0)
		return 0;

	if (pagesizes && (nr_sizes == n_elem))
		return nr_sizes;
	if (pagesizes)
		pagesizes[nr_sizes] = default_size;
	nr_sizes++;

	/*
	 * Scan sysfs to look for other sizes.
	 * Non-existing dir is not an error, we got one size from /proc/meminfo.
	 */
	sysfs = opendir(SYSFS_HUGEPAGES_DIR);
	if (!sysfs) {
		if (errno == ENOENT) {
			errno = 0;
			return nr_sizes;
		} else
			return -1;
	}
	while ((ent = readdir(sysfs))) {
		long size;

		if (strncmp(ent->d_name, "hugepages-", 10))
			continue;

		size = strtol(ent->d_name + 10, NULL, 10);
		if (size == LONG_MIN || size == LONG_MAX)
			continue;
		size = size_to_smaller_unit(size);

		if (size < 0 || size == default_size)
			continue;
		if (pagesizes && (nr_sizes == n_elem))
			return nr_sizes;
		if (pagesizes)
			pagesizes[nr_sizes] = size;
		nr_sizes++;
	}
	closedir(sysfs);

	return nr_sizes;
}

int getpagesizes(long pagesizes[], int n_elem)
{
	int ret;

	if (n_elem < 0 || (n_elem > 0 && pagesizes == NULL)) {
		errno = EINVAL;
		return -1;
	}

	/* Requests for sizing, we need one more slot than gethugepagesizes. */
	if (pagesizes == NULL && n_elem == 0) {
		ret = gethugepagesizes(pagesizes, n_elem);
	} else {
		/* Install the base page size. */
		if (pagesizes && n_elem == 0)
			return 0;
		if (pagesizes)
			pagesizes[0] = sysconf(_SC_PAGESIZE);

		ret = gethugepagesizes(pagesizes + 1, n_elem - 1);
	}
	if (ret < 0)
		return ret;
	return ret + 1;
}

int hugetlbfs_test_path(const char *mount)
{
	struct statfs64 sb;
	int err;

	/* Bugs in the 32<->64 translation code in pre-2.6.15 kernels
	 * mean that plain statfs() returns bogus errors on hugetlbfs
	 * filesystems.  Use statfs64() to work around. */
	err = statfs64(mount, &sb);
	if (err)
		return -1;

	return (sb.f_type == HUGETLBFS_MAGIC);
}

/* Return the page size for the given mount point in bytes */
long hugetlbfs_test_pagesize(const char *mount)
{
	struct statfs64 sb;
	int err;

	err = statfs64(mount, &sb);
	if (err)
		return -1;

	if ((sb.f_bsize <= 0) || (sb.f_bsize > LONG_MAX))
		return -1;

	return sb.f_bsize;
}

const char *hugetlbfs_find_path_for_size(long page_size)
{
	char *path;
	int idx;

	idx = hpage_size_to_index(page_size);
	if (idx >= 0) {
		path = hpage_sizes[idx].mount;
		if (strlen(path))
			return path;
	}
	return NULL;
}

const char *hugetlbfs_find_path(void)
{
	long hpage_size = gethugepagesize();
	if (hpage_size > 0)
		return hugetlbfs_find_path_for_size(hpage_size);
	else
		return NULL;
}

int hugetlbfs_unlinked_fd_for_size(long page_size)
{
	const char *path;
	char name[PATH_MAX+1];
	int fd;

	path = hugetlbfs_find_path_for_size(page_size);
	if (!path)
		return -1;

	name[sizeof(name)-1] = '\0';

	strcpy(name, path);
	strncat(name, "/libhugetlbfs.tmp.XXXXXX", sizeof(name)-1);
	/* FIXME: deal with overflows */

	fd = mkstemp64(name);

	if (fd < 0) {
		ERROR("mkstemp() failed: %s\n", strerror(errno));
		return -1;
	}

	unlink(name);

	return fd;
}

int hugetlbfs_unlinked_fd(void)
{
	long hpage_size = gethugepagesize();
	if (hpage_size > 0)
		return hugetlbfs_unlinked_fd_for_size(hpage_size);
	else
		return -1;
}

#define IOV_LEN 64
int hugetlbfs_prefault(void *addr, size_t length)
{
	size_t offset;
	struct iovec iov[IOV_LEN];
	int ret;
	int i;
	int fd;

	if (!__hugetlbfs_prefault)
		return 0;

	/*
	 * The NUMA users of libhugetlbfs' malloc feature are
	 * expected to use the numactl program to specify an
	 * appropriate policy for hugepage allocation
	 *
	 * Use readv(2) to instantiate the hugepages unless HUGETLB_NO_PREFAULT
	 * is set. If we instead returned a hugepage mapping with insufficient
	 * hugepages, the VM system would kill the process when the
	 * process tried to access the missing memory.
	 *
	 * The value of this environment variable is read during library
	 * initialisation and sets __hugetlbfs_prefault accordingly. If
	 * prefaulting is enabled and we can't get all that were requested,
	 * -ENOMEM is returned. The caller is expected to release the entire
	 * mapping and optionally it may recover by mapping base pages instead.
	 */

	fd = open("/dev/zero", O_RDONLY);
	if (fd < 0) {
		ERROR("Failed to open /dev/zero for reading\n");
		return -ENOMEM;
	}

	for (offset = 0; offset < length; ) {
		for (i = 0; i < IOV_LEN && offset < length; i++) {
			iov[i].iov_base = addr + offset;
			iov[i].iov_len = 1;
			offset += gethugepagesize();
		}
		ret = readv(fd, iov, i);
		if (ret != i) {
			DEBUG("Got %d of %d requested; err=%d\n", ret,
					i, ret < 0 ? errno : 0);
			WARNING("Failed to reserve %ld huge pages "
					"for new region\n",
					length / gethugepagesize());
			close(fd);
			return -ENOMEM;
		}
	}

	close(fd);
	return 0;
}

long get_huge_page_counter(long pagesize, unsigned int counter)
{
	char file[PATH_MAX+1];
	char *key;

	if (select_pool_counter(counter, pagesize, file, &key))
		return -1;

	if (access(file, O_RDONLY))
		return -1;

	return file_read_ulong(file, key);
}

int set_huge_page_counter(long pagesize, unsigned int counter,
			unsigned long val)
{
	char file[PATH_MAX+1];

	if (select_pool_counter(counter, pagesize, file, NULL))
		return -1;

	return file_write_ulong(file, val);
}

int set_nr_hugepages(long pagesize, unsigned long val)
{
	return set_huge_page_counter(pagesize, HUGEPAGES_TOTAL, val);
}

int set_nr_overcommit_hugepages(long pagesize, unsigned long val)
{
	DEBUG("setting HUGEPAGES_OC to %ld\n", val);
	return set_huge_page_counter(pagesize, HUGEPAGES_OC, val);
}

long read_nr_overcommit(long page_size)
{
	if (!kernel_has_overcommit())
		return -1;

	return get_huge_page_counter(page_size, HUGEPAGES_OC);
}

void restore_overcommit_pages(long page_size, long oc_pool)
{
	if (!kernel_has_overcommit())
		return;

	set_nr_overcommit_hugepages(page_size, oc_pool);
}

/********************************************************************/
/* Library user visible DIAGNOSES/DEBUGGING ONLY functions          */
/********************************************************************/

#define MAPS_BUF_SZ 4096
long dump_proc_pid_maps()
{
	FILE *f;
	char line[MAPS_BUF_SZ];
	size_t ret;

	f = fopen("/proc/self/maps", "r");
	if (!f) {
		ERROR("Failed to open /proc/self/maps\n");
		return -1;
	}

	while (1) {
		ret = fread(line, sizeof(char), MAPS_BUF_SZ, f);
		if (ret < 0) {
			ERROR("Failed to read /proc/self/maps\n");
			return -1;
		}
		if (ret == 0)
			break;
		ret = fwrite(line, sizeof(char), ret, stderr);
		if (ret < 0) {
			ERROR("Failed to write /proc/self/maps to stderr\n");
			return -1;
		}
	}

	fclose(f);
	return 0;
}

long read_meminfo(const char *tag)
{
	return file_read_ulong(MEMINFO, tag);
}

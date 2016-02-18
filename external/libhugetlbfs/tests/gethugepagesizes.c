/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 Adam Litke, IBM Corporation.
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <hugetlbfs.h>

#include "hugetests.h"

int faked_data = 0;
char fake_sysfs[] = "/tmp/sysfs-XXXXXX";
char fake_meminfo[] = "/tmp/meminfo-XXXXXX";

#define REAL_SYSFS_DIR	"/sys/kernel/mm/hugepages/"
DIR *(*real_opendir)(const char *name);

int (*real_open)(const char *name, int flags, int mode);

enum {
	OVERRIDE_OFF,		/* Pass-through to real function */
	OVERRIDE_ON,		/* Ovewrride with local function */
	OVERRIDE_MISSING,	/* Emulate missing support */
};
int meminfo_state = OVERRIDE_OFF;
int sysfs_state = OVERRIDE_OFF;

/*
 * Override opendir so we'll open the fake sysfs dir if intended
 */
DIR *opendir(const char *name)
{
	if (!real_opendir)
		real_opendir = dlsym(RTLD_NEXT, "opendir");

	/* Only override calls to the sysfs dir */
	if (strcmp(name, REAL_SYSFS_DIR))
		return real_opendir(name);

	switch (sysfs_state) {
	case OVERRIDE_OFF:
		return real_opendir(name);
	case OVERRIDE_ON:
		/* Only safe to override of fake_sysfs was set up */
		if (faked_data)
			return real_opendir(fake_sysfs);
		else
			FAIL("Trying to override opendir before initializing "
				"fake_sysfs directory\n");
	default:
		errno = ENOENT;
		return NULL;
	}
}

#define HPAGE_KB 2048
#define __HPAGE_STR_QUOTE(val) #val
#define __HPAGE_STR(val) __HPAGE_STR_QUOTE(val)
#define HPAGE_STR __HPAGE_STR(HPAGE_KB)

/*
 * Override open to simulate various contents for meminfo
 */
int open(const char *file, int flags, ...)
{
	int mode = 0;
	if (flags & O_CREAT) {
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);
	}

	if (!real_open)
		real_open = dlsym(RTLD_NEXT, "open");

	switch (meminfo_state) {
		case OVERRIDE_OFF:
			break;
		case OVERRIDE_ON: {
			char fname[PATH_MAX];
			sprintf(fname, "%s/meminfo-hugepages", fake_meminfo);
			file = fname;
			break;
		}
		case OVERRIDE_MISSING: {
			char fname[PATH_MAX];
			sprintf(fname, "%s/meminfo-none", fake_meminfo);
			file = fname;
			break;
		}
		default:
			return -1;
	}
	return real_open(file, flags, mode);
}

void cleanup_fake_data(void)
{
	DIR *dir;
	struct dirent *ent;
	char fname[PATH_MAX+1];

	meminfo_state = OVERRIDE_OFF;
	sysfs_state = OVERRIDE_OFF;

	faked_data = 0;
	dir = opendir(fake_sysfs);
	if (!dir)
		FAIL("opendir %s: %s", fake_sysfs, strerror(errno));

	while ((ent = readdir(dir))) {
		if (strncmp(ent->d_name, "hugepages-", 10))
			continue;
		snprintf(fname, PATH_MAX, "%s/%s", fake_sysfs,
			ent->d_name);
		if (rmdir(fname))
			FAIL("rmdir %s: %s", fake_sysfs, strerror(errno));
	}
	closedir(dir);
	if (rmdir(fake_sysfs))
		FAIL("rmdir %s: %s", fake_sysfs, strerror(errno));

	sprintf(fname, "%s/meminfo-none", fake_meminfo);
	if (unlink(fname) < 0)
		FAIL("unlink %s: %s", fname, strerror(errno));
	sprintf(fname, "%s/meminfo-hugepages", fake_meminfo);
	if (unlink(fname) < 0)
		FAIL("unlink %s: %s", fname, strerror(errno));
	if (rmdir(fake_meminfo))
		FAIL("rmdir %s: %s", fake_meminfo, strerror(errno));
}

char *meminfo_base = "\
MemTotal:      4004132 kB\n\
MemFree:       3563748 kB\n\
Buffers:         34804 kB\n\
Cached:         252544 kB\n\
SwapCached:          0 kB\n\
Active:         108912 kB\n\
Inactive:       187420 kB\n\
SwapTotal:     8008392 kB\n\
SwapFree:      8008392 kB\n\
Dirty:               4 kB\n\
Writeback:           0 kB\n\
AnonPages:        9100 kB\n\
Mapped:           7908 kB\n\
Slab:            40212 kB\n\
SReclaimable:    33312 kB\n\
SUnreclaim:       6900 kB\n\
PageTables:       1016 kB\n\
NFS_Unstable:        0 kB\n\
Bounce:              0 kB\n\
WritebackTmp:        0 kB\n\
CommitLimit:   9974616 kB\n\
Committed_AS:    29616 kB\n\
VmallocTotal: 34359738367 kB\n\
VmallocUsed:     23760 kB\n\
VmallocChunk: 34359714543 kB\n\
";

char *meminfo_huge = "\
HugePages_Total:    35\n\
HugePages_Free:     35\n\
HugePages_Rsvd:      0\n\
HugePages_Surp:      0\n\
Hugepagesize:     " HPAGE_STR " kB\n\
";

void setup_fake_data(long sizes[], int n_elem)
{
	int old_meminfo_state = meminfo_state;
	int old_sysfs_state = sysfs_state;

	int i;
	char fname[PATH_MAX+1];
	int fd;

	meminfo_state = OVERRIDE_OFF;
	sysfs_state = OVERRIDE_OFF;

	if (faked_data)
		cleanup_fake_data();

	/* Generate some fake sysfs data. */
	if (!mkdtemp(fake_sysfs))
		FAIL("mkdtemp: %s", strerror(errno));
	faked_data = 1;

	for (i = 0; i < n_elem; i++) {
		snprintf(fname, PATH_MAX, "%s/hugepages-%lukB", fake_sysfs,
				sizes[i] / 1024);
		if (mkdir(fname, 0700))
			FAIL("mkdir %s: %s", fname, strerror(errno));
	}

	/* Generate fake meminfo data. */
	if (!mkdtemp(fake_meminfo))
		FAIL("mkdtemp: %s", strerror(errno));

	sprintf(fname, "%s/meminfo-none", fake_meminfo);
	fd = open(fname, O_WRONLY|O_CREAT);
	if (fd < 0)
		FAIL("open: %s", strerror(errno));
	if (write(fd, meminfo_base,
			strlen(meminfo_base)) != strlen(meminfo_base))
		FAIL("write: %s", strerror(errno));
	if (close(fd) < 0)
		FAIL("close: %s", strerror(errno));

	sprintf(fname, "%s/meminfo-hugepages", fake_meminfo);
	fd = open(fname, O_WRONLY|O_CREAT);
	if (fd < 0)
		FAIL("open: %s", strerror(errno));
	if (write(fd, meminfo_base,
			strlen(meminfo_base)) != strlen(meminfo_base))
		FAIL("write: %s", strerror(errno));
	if (write(fd, meminfo_huge,
			strlen(meminfo_huge)) != strlen(meminfo_huge))
		FAIL("write: %s", strerror(errno));
	if (close(fd) < 0)
		FAIL("close: %s", strerror(errno));

	meminfo_state = old_meminfo_state;
	sysfs_state = old_sysfs_state;
}

void cleanup(void)
{
	if (faked_data)
		cleanup_fake_data();
}

void validate_sizes(int line, long actual_sizes[], int actual,
		    int max, int maxmax,
		    long expected_sizes[], int expected)
{
	int i, j;

	verbose_printf("Line %d: Expecting sizes:", line);
	for (i = 0; i < expected; i++)
		verbose_printf(" %ld", expected_sizes[i]);
	verbose_printf("\n");
	verbose_printf("Line %d: Actual sizes are:", line);
	for (i = 0; i < actual; i++)
		verbose_printf(" %ld", actual_sizes[i]);
	verbose_printf("\n");

	if (((expected <= max) && (expected != actual))
	    || ((expected > max) && (actual < max)))
		FAIL("Line %i: Wrong number of sizes returned -- expected %i "
		     "got %i", line, expected, actual);
	else if (actual > max)
		FAIL("Line %i: %i sizes returned > maximum %i",
		     line, actual, max);

	for (i = 0; i < actual; i++) {
		for (j = 0; j < expected; j++)
			if (actual_sizes[i] == expected_sizes[j])
				break;
		if (j >= expected)
			FAIL("Line %i: Actual size %li not found in expected "
				"results", line, expected_sizes[i]);
	}

	for (i = 0; i < actual; i++)
		for (j = i+1; j < actual; j++)
			if (actual_sizes[i] == actual_sizes[j])
				FAIL("Line %i: Duplicate size %li at %i/%i",
				     line, actual_sizes[i], i, j);

	for (i = actual; i < maxmax; i++)
		if (actual_sizes[i] != 42)
			FAIL("Line %i: Wrote past official limit at %i",
				line, i);
}

#define MAX 16
#define EXPECT_SIZES(func, max, count, expected)			\
({									\
	long __a[MAX] = { [0 ... MAX-1] = 42 };				\
	int __na;							\
									\
	__na = func(__a, max);						\
									\
	validate_sizes(__LINE__, __a, __na, max, MAX, expected, count);	\
									\
	__na;								\
})

#define INIT_LIST(a, values...)						\
({									\
	long __e[] = { values };					\
	memcpy(a, __e, sizeof(__e));					\
})

int main(int argc, char *argv[])
{
	int i, fakes_no;
	long expected_sizes[MAX], actual_sizes[MAX], fake_sizes[MAX];
	long base_size = sysconf(_SC_PAGESIZE);

	test_init(argc, argv);

	/*
	 * ===
	 * Argment error checking tests
	 * ===
	 */
	meminfo_state = OVERRIDE_OFF;
	sysfs_state = OVERRIDE_OFF;
	kernel_default_hugepage_size_reset();

	if (gethugepagesizes(actual_sizes, -1) != -1 || errno != EINVAL)
		FAIL("Mishandled params (n_elem < 0)");
	if (gethugepagesizes(NULL, 1) != -1 || errno != EINVAL)
		FAIL("Mishandled params (pagesizes == NULL, n_elem > 0)");

	if (getpagesizes(actual_sizes, -1) != -1 || errno != EINVAL)
		FAIL("Mishandled params (n_elem < 0)");
	if (getpagesizes(NULL, 1) != -1 || errno != EINVAL)
		FAIL("Mishandled params (pagesizes == NULL, n_elem > 0)");

	/*
	 * ===
	 * Test some corner cases using a fake system configuration
	 * ===
	 */

	INIT_LIST(expected_sizes, HPAGE_KB * 1024, 1024 * 1024, 64 * 1024);
	fakes_no = 0;
	for (i = 0; i < 3; i++)
		/* don't include base_size in 'fake' hugepagesizes */
		if (base_size != expected_sizes[i]) {
			fake_sizes[fakes_no] = expected_sizes[i];
			fakes_no++;
		}
	setup_fake_data(fake_sizes, fakes_no);

	/*
	 * Check handling when /proc/meminfo indicates no huge page support
	 * and the sysfs heirachy is not present.
	 */
	meminfo_state = OVERRIDE_MISSING;
	sysfs_state = OVERRIDE_MISSING;
	kernel_default_hugepage_size_reset();

	EXPECT_SIZES(gethugepagesizes, MAX, 0, expected_sizes);

	INIT_LIST(expected_sizes, base_size);
	EXPECT_SIZES(getpagesizes, MAX, 1, expected_sizes);

	/* ... only the meminfo size is returned. */
	meminfo_state = OVERRIDE_ON;
	kernel_default_hugepage_size_reset();

	INIT_LIST(expected_sizes, HPAGE_KB * 1024);
	EXPECT_SIZES(gethugepagesizes, MAX, 1, expected_sizes);

	INIT_LIST(expected_sizes, base_size, HPAGE_KB * 1024);
	EXPECT_SIZES(getpagesizes, MAX, 2, expected_sizes);

	/*
	 * When sysfs defines additional sizes ...
	 */
	sysfs_state = OVERRIDE_ON;
	kernel_default_hugepage_size_reset();

	memcpy(expected_sizes, fake_sizes, sizeof(fake_sizes));

	/* ... make sure all sizes are returned without duplicates */
	/* ... while making sure we do not overstep our limit */
	EXPECT_SIZES(gethugepagesizes, MAX, fakes_no, expected_sizes);
	EXPECT_SIZES(gethugepagesizes, 1, fakes_no, expected_sizes);
	EXPECT_SIZES(gethugepagesizes, 2, fakes_no, expected_sizes);
	EXPECT_SIZES(gethugepagesizes, 3, fakes_no, expected_sizes);
	EXPECT_SIZES(gethugepagesizes, 4, fakes_no, expected_sizes);

	memcpy(expected_sizes, fake_sizes, sizeof(fake_sizes));
	expected_sizes[fakes_no] = base_size;
	EXPECT_SIZES(getpagesizes, MAX, fakes_no + 1, expected_sizes);
	EXPECT_SIZES(getpagesizes, 1, fakes_no + 1, expected_sizes);
	EXPECT_SIZES(getpagesizes, 2, fakes_no + 1, expected_sizes);
	EXPECT_SIZES(getpagesizes, 3, fakes_no + 1, expected_sizes);
	EXPECT_SIZES(getpagesizes, 4, fakes_no + 1, expected_sizes);
	EXPECT_SIZES(getpagesizes, 5, fakes_no + 1, expected_sizes);

	/* ... we can check how many sizes are supported. */
	if (gethugepagesizes(NULL, 0) != fakes_no)
		FAIL("Unable to check the number of supported sizes");

	if (getpagesizes(NULL, 0) != fakes_no + 1)
		FAIL("Unable to check the number of supported sizes");

	PASS();
}

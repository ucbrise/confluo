/***************************************************************************
 *   User front end for using huge pages Copyright (C) 2008, IBM           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the Lesser GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of the  *
 *   License, or at your option) any later version.                        *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                   *
 *                                                                         *
 *   You should have received a copy of the Lesser GNU General Public      *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * hugeadm is designed to make an administrators life simpler, to automate
 * and simplify basic system configuration as it relates to hugepages.  It
 * is designed to help with pool and mount configuration.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <mntent.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/swap.h>
#include <sys/wait.h>

#define _GNU_SOURCE /* for getopt_long */
#include <unistd.h>
#include <getopt.h>

#define KB (1024)
#define MB (1024*KB)
#define GB (1024*MB)

#define REPORT_UTIL "hugeadm"
#define REPORT(level, prefix, format, ...)                                    \
	do {                                                                  \
		if (verbose_level >= level)                                   \
			fprintf(stderr, "hugeadm:" prefix ": " format,       \
				##__VA_ARGS__);                               \
	} while (0);

#include "libhugetlbfs_internal.h"
#include "hugetlbfs.h"

extern int optind;
extern char *optarg;

#define OPTION(opts, text)	fprintf(stderr, " %-25s  %s\n", opts, text)
#define CONT(text) 		fprintf(stderr, " %-25s  %s\n", "", text)

#define MOUNT_DIR "/var/lib/hugetlbfs"
#define OPT_MAX 4096

#define PROCMOUNTS "/proc/mounts"
#define PROCHUGEPAGES_MOVABLE "/proc/sys/vm/hugepages_treat_as_movable"
#define PROCMINFREEKBYTES "/proc/sys/vm/min_free_kbytes"
#define PROCSHMMAX "/proc/sys/kernel/shmmax"
#define PROCHUGETLBGROUP "/proc/sys/vm/hugetlb_shm_group"
#define PROCZONEINFO "/proc/zoneinfo"
#define FS_NAME "hugetlbfs"
#define MIN_COL 20
#define MAX_SIZE_MNTENT (64 + PATH_MAX + 32 + 128 + 2 * sizeof(int))
#define FORMAT_LEN 20

#define MEM_TOTAL "MemTotal:"
#define SWAP_FREE "SwapFree:"
#define SWAP_TOTAL "SwapTotal:"

#define ALWAYS		  "always"
#define MADVISE		  "madvise"
#define NEVER		  "never"
#define TRANS_ENABLE	  "/sys/kernel/mm/transparent_hugepage/enabled"
#define KHUGE_SCAN_PAGES  "/sys/kernel/mm/transparent_hugepage/khugepaged/pages_to_scan"
#define KHUGE_SCAN_SLEEP  "/sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs"
#define KHUGE_ALLOC_SLEEP "/sys/kernel/mm/transparent_hugepage/khugepaged/alloc_sleep_millisecs"

void print_usage()
{
	fprintf(stderr, "hugeadm [options]\n");
	fprintf(stderr, "options:\n");

	OPTION("--list-all-mounts", "List all current hugetlbfs mount points");
	OPTION("--pool-list", "List all pools");
	OPTION("--hard", "specified with --pool-pages-min to make");
	CONT("multiple attempts at adjusting the pool size to the");
	CONT("specified count on failure");
	OPTION("--pool-pages-min <size|DEFAULT>:[+|-]<pagecount|memsize<G|M|K>>", "");
	CONT("Adjust pool 'size' lower bound");
	OPTION("--obey-mempolicy", "Obey the NUMA memory policy when");
	CONT("adjusting the pool 'size' lower bound");
	OPTION("--thp-always", "Enable transparent huge pages always");
	OPTION("--thp-madvise", "Enable transparent huge pages with madvise");
	OPTION("--thp-never", "Disable transparent huge pages");
	OPTION("--thp-khugepaged-pages <pages to scan>", "Number of pages that khugepaged");
	CONT("should scan on each pass");
	OPTION("--thp-khugepaged-scan-sleep <milliseconds>", "Time in ms to sleep between");
	CONT("khugepaged passes");
	OPTION("--thp-khugepages-alloc-sleep <milliseconds>", "Time in ms for khugepaged");
	CONT("to wait if there was a huge page allocation failure");
	OPTION("--pool-pages-max <size|DEFAULT>:[+|-]<pagecount|memsize<G|M|K>>", "");
	CONT("Adjust pool 'size' upper bound");
	OPTION("--set-recommended-min_free_kbytes", "");
	CONT("Sets min_free_kbytes to a recommended value to improve availability of");
	CONT("huge pages at runtime");
	OPTION("--set-recommended-shmmax", "Sets shmmax to a recommended value to");
	CONT("maximise the size possible for shared memory pools");
	OPTION("--set-shm-group <gid|groupname>", "Sets hugetlb_shm_group to the");
	CONT("specified group, which has permission to use hugetlb shared memory pools");
	OPTION("--add-temp-swap[=count]", "Specified with --pool-pages-min to create");
	CONT("temporary swap space for the duration of the pool resize. Default swap");
	CONT("size is 5 huge pages. Optional arg sets size to 'count' huge pages");
	OPTION("--add-ramdisk-swap", "Specified with --pool-pages-min to create");
	CONT("swap space on ramdisks. By default, swap is removed after the resize.");
	OPTION("--persist", "Specified with --add-temp-swap or --add-ramdisk-swap");
	CONT("options to make swap space persist after the resize.");
	OPTION("--enable-zone-movable", "Use ZONE_MOVABLE for huge pages");
	OPTION("--disable-zone-movable", "Do not use ZONE_MOVABLE for huge pages");
	OPTION("--create-mounts", "Creates a mount point for each available");
	CONT("huge page size on this system under /var/lib/hugetlbfs");
	OPTION("--create-user-mounts <user>", "");
	CONT("Creates a mount point for each available huge");
	CONT("page size under /var/lib/hugetlbfs/<user>");
	CONT("usable by user <user>");
	OPTION("--create-group-mounts <group>", "");
	CONT("Creates a mount point for each available huge");
	CONT("page size under /var/lib/hugetlbfs/<group>");
	CONT("usable by group <group>");
	OPTION("--create-global-mounts", "");
	CONT("Creates a mount point for each available huge");
	CONT("page size under /var/lib/hugetlbfs/global");
	CONT("usable by anyone");

	OPTION("--max-size <size<G|M|K>>", "Limit the filesystem size of a new mount point");
	OPTION("--max-inodes <number>", "Limit the number of inodes on a new mount point");

	OPTION("--page-sizes", "Display page sizes that a configured pool");
	OPTION("--page-sizes-all",
			"Display page sizes support by the hardware");
	OPTION("--dry-run", "Print the equivalent shell commands for what");
	CONT("the specified options would have done without");
	CONT("taking any action");

	OPTION("--explain", "Gives a overview of the status of the system");
	CONT("with respect to huge page availability");

	OPTION("--verbose <level>, -v", "Increases/sets tracing levels");
	OPTION("--help, -h", "Prints this message");
}

int opt_dry_run = 0;
int opt_hard = 0;
int opt_movable = -1;
int opt_set_recommended_minfreekbytes = 0;
int opt_set_recommended_shmmax = 0;
int opt_set_hugetlb_shm_group = 0;
int opt_temp_swap = 0;
int opt_ramdisk_swap = 0;
int opt_swap_persist = 0;
int opt_obey_mempolicy = 0;
unsigned long opt_limit_mount_size = 0;
int opt_limit_mount_inodes = 0;
int verbose_level = VERBOSITY_DEFAULT;
char ramdisk_list[PATH_MAX] = "";

void setup_environment(char *var, char *val)
{
	if (opt_dry_run) {
		printf("%s='%s'\n", var, val);
		return;
	}

	setenv(var, val, 1);
	DEBUG("%s='%s'\n", var, val);
}

/* Enable/disable allocation of hugepages from ZONE_MOVABLE */
void setup_zone_movable(int able)
{
	if (opt_dry_run) {
		printf("echo %d > %s\n", able, PROCHUGEPAGES_MOVABLE);
		return;
	}

	DEBUG("Setting %s to %d\n", PROCHUGEPAGES_MOVABLE, able);

	/* libhugetlbfs reports any error that occurs */
	file_write_ulong(PROCHUGEPAGES_MOVABLE, (unsigned long)able);
}

void verbose_init(void)
{
	char *env;

	env = getenv("HUGETLB_VERBOSE");
	if (env)
		verbose_level = atoi(env);
	env = getenv("HUGETLB_DEBUG");
	if (env)
		verbose_level = VERBOSITY_MAX;
}

void verbose(char *which)
{
	int new_level;

	if (which) {
		new_level = atoi(which);
		if (new_level < 0 || new_level > 99) {
			ERROR("%d: verbosity out of range 0-99\n",
				new_level);
			exit(EXIT_FAILURE);
		}
	} else {
		new_level = verbose_level + 1;
		if (new_level == 100) {
			WARNING("verbosity limited to 99\n");
			new_level--;
		}
	}
	verbose_level = new_level;
}

void verbose_expose(void)
{
	char level[3];

	if (verbose_level == 99) {
		setup_environment("HUGETLB_DEBUG", "yes");
	}
	snprintf(level, sizeof(level), "%d", verbose_level);
	setup_environment("HUGETLB_VERBOSE", level);
}

/*
 * getopts return values for options which are long only.
 */
#define LONG_POOL		('p' << 8)
#define LONG_POOL_LIST		(LONG_POOL|'l')
#define LONG_POOL_MIN_ADJ	(LONG_POOL|'m')
#define LONG_POOL_MAX_ADJ	(LONG_POOL|'M')
#define LONG_POOL_MEMPOL	(LONG_POOL|'p')

#define LONG_SET_RECOMMENDED_MINFREEKBYTES	('k' << 8)
#define LONG_SET_RECOMMENDED_SHMMAX		('x' << 8)
#define LONG_SET_HUGETLB_SHM_GROUP		('R' << 8)

#define LONG_MOVABLE		('z' << 8)
#define LONG_MOVABLE_ENABLE	(LONG_MOVABLE|'e')
#define LONG_MOVABLE_DISABLE	(LONG_MOVABLE|'d')

#define LONG_HARD		('h' << 8)
#define LONG_SWAP		('s' << 8)
#define LONG_SWAP_DISK		(LONG_SWAP|'d')
#define LONG_SWAP_RAMDISK	(LONG_SWAP|'r')
#define LONG_SWAP_PERSIST	(LONG_SWAP|'p')

#define LONG_PAGE	('P' << 8)
#define LONG_PAGE_SIZES	(LONG_PAGE|'s')
#define LONG_PAGE_AVAIL	(LONG_PAGE|'a')

#define LONG_MOUNTS			('m' << 8)
#define LONG_CREATE_MOUNTS		(LONG_MOUNTS|'C')
#define LONG_CREATE_USER_MOUNTS		(LONG_MOUNTS|'U')
#define LONG_CREATE_GROUP_MOUNTS	(LONG_MOUNTS|'g')
#define LONG_CREATE_GLOBAL_MOUNTS	(LONG_MOUNTS|'G')
#define LONG_LIST_ALL_MOUNTS		(LONG_MOUNTS|'A')

#define LONG_LIMITS			('l' << 8)
#define LONG_LIMIT_SIZE			(LONG_LIMITS|'S')
#define LONG_LIMIT_INODES		(LONG_LIMITS|'I')

#define LONG_EXPLAIN	('e' << 8)

#define LONG_TRANS			('t' << 8)
#define LONG_TRANS_ALWAYS		(LONG_TRANS|'a')
#define LONG_TRANS_MADVISE		(LONG_TRANS|'m')
#define LONG_TRANS_NEVER		(LONG_TRANS|'n')

#define LONG_KHUGE			('K' << 8)
#define LONG_KHUGE_PAGES		(LONG_KHUGE|'p')
#define LONG_KHUGE_SCAN			(LONG_KHUGE|'s')
#define LONG_KHUGE_ALLOC		(LONG_KHUGE|'a')

#define MAX_POOLS	32

static int cmpsizes(const void *p1, const void *p2)
{
	return ((struct hpage_pool *)p1)->pagesize >
			((struct hpage_pool *)p2)->pagesize;
}

void pool_list(void)
{
	struct hpage_pool pools[MAX_POOLS];
	int pos;
	int cnt;

	cnt = hpool_sizes(pools, MAX_POOLS);
	if (cnt < 0) {
		ERROR("unable to obtain pools list");
		exit(EXIT_FAILURE);
	}
	qsort(pools, cnt, sizeof(pools[0]), cmpsizes);

	printf("%10s %8s %8s %8s %8s\n",
		"Size", "Minimum", "Current", "Maximum", "Default");
	for (pos = 0; cnt--; pos++) {
		printf("%10ld %8ld %8ld %8ld %8s\n", pools[pos].pagesize,
			pools[pos].minimum, pools[pos].size,
			pools[pos].maximum, (pools[pos].is_default) ? "*" : "");
	}
}

struct mount_list
{
	struct mntent entry;
	char data[MAX_SIZE_MNTENT];
	struct mount_list *next;
};

void print_mounts(struct mount_list *current, int longest)
{
	char format_str[FORMAT_LEN];

	snprintf(format_str, FORMAT_LEN, "%%-%ds %%s\n", longest);
	printf(format_str, "Mount Point", "Options");
	while (current) {
		printf(format_str, current->entry.mnt_dir,
				   current->entry.mnt_opts);
		current = current->next;
	}
}

/*
 * collect_active_mounts returns a list of active hugetlbfs
 * mount points, and, if longest is not NULL, the number of
 * characters in the longest mount point to ease output
 * formatting.  Caller is expected to free the list of mounts.
 */
struct mount_list *collect_active_mounts(int *longest)
{
	FILE *mounts;
	struct mount_list *list, *current, *previous = NULL;
	int length;

	/* First try /proc/mounts, then /etc/mtab */
	mounts = setmntent(PROCMOUNTS, "r");
	if (!mounts) {
		mounts = setmntent(MOUNTED, "r");
		if (!mounts) {
			ERROR("unable to open %s or %s for reading",
				PROCMOUNTS, MOUNTED);
			exit(EXIT_FAILURE);
		}
	}

	list = malloc(sizeof(struct mount_list));
	if (!list) {
		ERROR("out of memory");
		exit(EXIT_FAILURE);
	}

	list->next = NULL;
	current = list;
	while (getmntent_r(mounts, &(current->entry), current->data, MAX_SIZE_MNTENT)) {
		if (strcasecmp(current->entry.mnt_type, FS_NAME) == 0) {
			length = strlen(current->entry.mnt_dir);
			if (longest && length > *longest)
				*longest = length;

			current->next = malloc(sizeof(struct mount_list));
			if (!current->next) {
				ERROR("out of memory");
				exit(EXIT_FAILURE);
			}
			previous = current;
			current = current->next;
			current->next = NULL;
		}
	}

	endmntent(mounts);

	if (previous) {
		free(previous->next);
		previous->next = NULL;
		return list;
	}
	return NULL;
}

void mounts_list_all(void)
{
	struct mount_list *list, *previous;
	int longest = MIN_COL;

	list = collect_active_mounts(&longest);

	if (!list) {
		ERROR("No hugetlbfs mount points found\n");
		return;
	}

	print_mounts(list, longest);

	while (list) {
		previous = list;
		list = list->next;
		free(previous);
	}
}

int make_dir(char *path, mode_t mode, uid_t uid, gid_t gid)
{
	struct passwd *pwd;
	struct group *grp;

	if (opt_dry_run) {
		pwd = getpwuid(uid);
		grp = getgrgid(gid);
		printf("if [ ! -e %s ]\n", path);
		printf("then\n");
		printf(" mkdir %s\n", path);
		printf(" chown %s:%s %s\n", pwd->pw_name, grp->gr_name, path);
		printf(" chmod %o %s\n", mode, path);
		printf("fi\n");
		return 0;
	}

	if (mkdir(path, mode)) {
		if (errno != EEXIST) {
			ERROR("Unable to create dir %s, error: %s\n",
				path, strerror(errno));
			return 1;
		}
	} else {
		if (chown(path, uid, gid)) {
			ERROR("Unable to change ownership of %s, error: %s\n",
				path, strerror(errno));
			return 1;
		}

		if (chmod(path, mode)) {
			ERROR("Unable to change permission on %s, error: %s\n",
				path, strerror(errno));
			return 1;
		}
	}

	return 0;
}

/**
 * ensure_dir will build the entire directory structure up to and
 * including path, all directories built will be owned by
 * user:group and permissions will be set to mode.
 */
int ensure_dir(char *path, mode_t mode, uid_t uid, gid_t gid)
{
	char *idx;

	if (!path || strlen(path) == 0)
		return 0;

	idx = strchr(path + 1, '/');

	do {
		if (idx)
			*idx = '\0';

		if (make_dir(path, mode, uid, gid))
			return 1;

		if (idx) {
			*idx = '/';
			idx++;
		}
	} while ((idx = strchr(idx, '/')) != NULL);

	if (make_dir(path, mode, uid, gid))
		return 1;

	return 0;
}

int check_if_already_mounted(struct mount_list *list, char *path)
{
	while (list) {
		if (!strcmp(list->entry.mnt_dir, path))
			return 1;
		list = list->next;
	}
	return 0;
}

int mount_dir(char *path, char *options, mode_t mode, uid_t uid, gid_t gid)
{
	struct passwd *pwd;
	struct group *grp;
	struct mntent entry;
	FILE *mounts;
        char dummy;
        int useMtab;

	struct mount_list *list, *previous;

	list = collect_active_mounts(NULL);

	if (list && check_if_already_mounted(list, path)) {
		WARNING("Directory %s is already mounted.\n", path);

		while (list) {
			previous = list;
			list = list->next;
			free(previous);
		}
		return 0;
	}

	while (list) {
		previous = list;
		list = list->next;
		free(previous);
	}

	if (opt_dry_run) {
		pwd = getpwuid(uid);
		grp = getgrgid(gid);
		printf("mount -t %s none %s -o %s\n", FS_NAME,
			path, options);
		printf("chown %s:%s %s\n", pwd->pw_name, grp->gr_name,
			path);
		printf("chmod %o %s\n", mode, path);
	} else {
		if (mount("none", path, FS_NAME, 0, options)) {
			ERROR("Unable to mount %s, error: %s\n",
				path, strerror(errno));
			return 1;
		}

          /* Check if mtab is a symlink */
          useMtab = (readlink(MOUNTED, &dummy, 1) < 0);
          if (useMtab) {
			mounts = setmntent(MOUNTED, "a+");
			if (mounts) {
				entry.mnt_fsname = FS_NAME;
				entry.mnt_dir = path;
				entry.mnt_type = FS_NAME;
				entry.mnt_opts = options;
				entry.mnt_freq = 0;
				entry.mnt_passno = 0;
				if (addmntent(mounts, &entry))
					WARNING("Unable to add entry %s to %s, error: %s\n",
						path, MOUNTED, strerror(errno));
				endmntent(mounts);
			} else {
				WARNING("Unable to open %s, error: %s\n",
					MOUNTED, strerror(errno));
			}
		}

		if (chown(path, uid, gid)) {
			ERROR("Unable to change ownership of %s, error: %s\n",
				path, strerror(errno));
			return 1;
		}

		if (chmod(path, mode)) {
			ERROR("Unable to set permissions on %s, error: %s\n",
				path, strerror(errno));
			return 1;
		}
	}
	return 0;
}

void scale_size(char *buf, unsigned long pagesize)
{
	if(pagesize >= GB)
		snprintf(buf, OPT_MAX, "%luGB", pagesize / GB);
	else if(pagesize >= MB)
		snprintf(buf, OPT_MAX, "%luMB", pagesize / MB);
	else
		snprintf(buf, OPT_MAX, "%luKB", pagesize / KB);
}

void create_mounts(char *user, char *group, char *base, mode_t mode)
{
	struct hpage_pool pools[MAX_POOLS];
	char path[PATH_MAX];
	char options[OPT_MAX];
	char limits[OPT_MAX];
	char scaled[OPT_MAX];
	int cnt, pos;
	struct passwd *pwd;
	struct group *grp;
	uid_t uid = 0;
	gid_t gid = 0;

	if (geteuid() != 0) {
		ERROR("Mounts can only be created by root\n");
		exit(EXIT_FAILURE);
	}

	if (user) {
		pwd = getpwnam(user);
		if (!pwd) {
			ERROR("Could not find specified user %s\n", user);
			exit(EXIT_FAILURE);
		}
		uid = pwd->pw_uid;
	} else if (group) {
		grp = getgrnam(group);
		if (!grp) {
			ERROR("Could not find specified group %s\n", group);
			exit(EXIT_FAILURE);
		}
		gid = grp->gr_gid;
	}

	if (ensure_dir(base,
		S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, 0, 0))
		exit(EXIT_FAILURE);

	cnt = hpool_sizes(pools, MAX_POOLS);
	if (cnt < 0) {
		ERROR("Unable to obtain pools list\n");
		exit(EXIT_FAILURE);
	}

	for (pos=0; cnt--; pos++) {
		scaled[0] = 0;
		scale_size(scaled, pools[pos].pagesize);
		if (user)
			snprintf(path, PATH_MAX, "%s/%s/pagesize-%s",
				base, user, scaled);
		else if (group)
			snprintf(path, PATH_MAX, "%s/%s/pagesize-%s",
				base, group, scaled);
		else
			snprintf(path, PATH_MAX, "%s/pagesize-%s",
				base, scaled);

		snprintf(options, OPT_MAX, "pagesize=%ld",
				pools[pos].pagesize);

		/* Yes, this could be cleverer */
		if (opt_limit_mount_size && opt_limit_mount_inodes)
			snprintf(limits, OPT_MAX, ",size=%lu,nr_inodes=%d",
				opt_limit_mount_size, opt_limit_mount_inodes);
		else {
			if (opt_limit_mount_size)
				snprintf(limits, OPT_MAX, ",size=%lu",
					opt_limit_mount_size);
			if (opt_limit_mount_inodes)
				snprintf(limits, OPT_MAX, ",nr_inodes=%d",
					opt_limit_mount_inodes);
		}

		/* Append limits if specified */
		if (limits[0] != 0) {
			size_t maxlen = OPT_MAX - strlen(options);
			if (maxlen > strlen(limits))
				strcat(options, limits);
			else
				WARNING("String limitations met, cannot append limitations onto mount options string. Increase OPT_MAX");
		}

		if (ensure_dir(path, mode, uid, gid))
			exit(EXIT_FAILURE);

		if (mount_dir(path, options, mode, uid, gid))
			exit(EXIT_FAILURE);
	}
}

/**
 * show_mem shouldn't change the behavior of any of its
 * callers, it only prints a message to the user showing the
 * total amount of memory in the system (in megabytes).
 */
void show_mem()
{
	long mem_total;

	mem_total = read_meminfo(MEM_TOTAL);
	printf("Total System Memory: %ld MB\n\n", mem_total / 1024);
}

/**
 * check_swap shouldn't change the behavior of any of its
 * callers, it only prints a message to the user if something
 * is being done that might fail without swap available.  i.e.
 * resizing a huge page pool
 */
void check_swap()
{
	long swap_sz;
	long swap_total;

	swap_total = read_meminfo(SWAP_TOTAL);
	if (swap_total <= 0) {
		WARNING("There is no swap space configured, resizing hugepage pool may fail\n");
		WARNING("Use --add-temp-swap option to temporarily add swap during the resize\n");
		return;
	}

	swap_sz = read_meminfo(SWAP_FREE);
	/* meminfo keeps values in kb, but we use bytes for hpage sizes */
	swap_sz *= 1024;
	if (swap_sz <= gethugepagesize()) {
		WARNING("There is very little swap space free, resizing hugepage pool may fail\n");
		WARNING("Use --add-temp-swap option to temporarily add swap during the resize\n");
	}
}

#define ZONEINFO_LINEBUF 1024
long recommended_minfreekbytes(void)
{
	FILE *f;
	char buf[ZONEINFO_LINEBUF];
	int nr_zones = 0;
	long recommended_min;
	long pageblock_kbytes = kernel_default_hugepage_size() / 1024;

	/* Detect the number of zones in the system */
	f = fopen(PROCZONEINFO, "r");
	if (f == NULL) {
		WARNING("Unable to open " PROCZONEINFO);
		return 0;
	}
	while (fgets(buf, ZONEINFO_LINEBUF, f) != NULL) {
		if (strncmp(buf, "Node ", 5) == 0)
			nr_zones++;
	}
	fclose(f);

	/* Make sure at least 2 pageblocks are free for MIGRATE_RESERVE */
	recommended_min = pageblock_kbytes * nr_zones * 2;

	/*
	 * Make sure that on average at least two pageblocks are almost free
	 * of another type, one for a migratetype to fall back to and a
	 * second to avoid subsequent fallbacks of other types There are 3
	 * MIGRATE_TYPES we care about.
	 */
	recommended_min += pageblock_kbytes * nr_zones * 3 * 3;
	return recommended_min;
}

void set_recommended_minfreekbytes(void)
{
	long recommended_min = recommended_minfreekbytes();

	if (opt_dry_run) {
		printf("echo \"%ld\" > %s\n", recommended_min,
			PROCMINFREEKBYTES);
		return;
	}

	DEBUG("Setting min_free_kbytes to %ld\n", recommended_min);
	file_write_ulong(PROCMINFREEKBYTES, (unsigned long)recommended_min);
}

/*
 * check_minfreekbytes does not alter the value of min_free_kbytes. It just
 * reports what the current value is and what it should be
 */
void check_minfreekbytes(void)
{
	long min_free_kbytes = file_read_ulong(PROCMINFREEKBYTES, NULL);
	long recommended_min = recommended_minfreekbytes();

	/* There should be at least one pageblock free per zone in the system */
	if (recommended_min > min_free_kbytes) {
		printf("\n");
		printf("The " PROCMINFREEKBYTES " of %ld is too small. To maximiuse efficiency\n", min_free_kbytes);
		printf("of fragmentation avoidance, there should be at least one huge page free per zone\n");
		printf("in the system which minimally requires a min_free_kbytes value of %ld\n", recommended_min);
	}
}

unsigned long long recommended_shmmax(void)
{
	struct hpage_pool pools[MAX_POOLS];
	unsigned long long recommended_shmmax = 0;
	int pos, cnt;

	cnt = hpool_sizes(pools, MAX_POOLS);
	if (cnt < 0) {
		ERROR("unable to obtain pools list");
		exit(EXIT_FAILURE);
	}

	for (pos = 0; cnt--; pos++)
		recommended_shmmax += ((unsigned long long)pools[pos].maximum *
							pools[pos].pagesize);

	return recommended_shmmax;
}

void set_recommended_shmmax(void)
{
	int ret;
	unsigned long max_recommended = -1UL;
	unsigned long long recommended = recommended_shmmax();

	if (recommended == 0) {
		printf("\n");
		WARNING("We can only set a recommended shmmax when huge pages are configured!\n");
		return;
	}

	if (recommended > max_recommended)
		recommended = max_recommended;

	DEBUG("Setting shmmax to %llu\n", recommended);
	ret = file_write_ulong(PROCSHMMAX, (unsigned long)recommended);

	if (!ret) {
		INFO("To make shmmax settings persistent, add the following line to /etc/sysctl.conf:\n");
		INFO("  kernel.shmmax = %llu\n", recommended);
	}
}

void check_shmmax(void)
{
	long current_shmmax = file_read_ulong(PROCSHMMAX, NULL);
	long recommended = recommended_shmmax();

	if (current_shmmax != recommended) {
		printf("\n");
		printf("A " PROCSHMMAX " value of %ld bytes may be sub-optimal. To maximise\n", current_shmmax);
		printf("shared memory usage, this should be set to the size of the largest shared memory\n");
		printf("segment size you want to be able to use. Alternatively, set it to a size matching\n");
		printf("the maximum possible allocation size of all huge pages. This can be done\n");
		printf("automatically, using the --set-recommended-shmmax option.\n");
	}

	if (recommended == 0) {
		printf("\n");
		WARNING("We can't make a shmmax recommendation until huge pages are configured!\n");
		return;
	}

	printf("\n");
	printf("The recommended shmmax for your currently allocated huge pages is %ld bytes.\n", recommended);
	printf("To make shmmax settings persistent, add the following line to /etc/sysctl.conf:\n");
	printf("  kernel.shmmax = %ld\n", recommended);
}

void set_hugetlb_shm_group(gid_t gid, char *group)
{
	int ret;

	DEBUG("Setting hugetlb_shm_group to %d (%s)\n", gid, group);
	ret = file_write_ulong(PROCHUGETLBGROUP, (unsigned long)gid);

	if (!ret) {
		INFO("To make hugetlb_shm_group settings persistent, add the following line to /etc/sysctl.conf:\n");
		INFO("  vm.hugetlb_shm_group = %d\n", gid);
	}
}

/* heisted from shadow-utils/libmisc/list.c::is_on_list() */
static int user_in_group(char *const *list, const char *member)
{
	while (*list != NULL) {
		if (strcmp(*list, member) == 0) {
			return 1;
		}
		list++;
	}

	return 0;
}

void check_user(void)
{
	uid_t uid;
	gid_t gid;
	struct passwd *pwd;
	struct group *grp;

	gid = (gid_t)file_read_ulong(PROCHUGETLBGROUP, NULL);
	grp = getgrgid(gid);
	if (!grp) {
		printf("\n");
		WARNING("Group ID %d in hugetlb_shm_group doesn't appear to be a valid group!\n", gid);
		return;
	}

	uid = getuid();
	pwd = getpwuid(uid);

	/* Don't segfault if user does not have a passwd entry. */
	if (!pwd) {
		printf("\n");
		WARNING("User uid %d is not in the password file!\n", uid);
		return;
	}

	if (gid != pwd->pw_gid && !user_in_group(grp->gr_mem, pwd->pw_name) && uid != 0) {
		printf("\n");
		WARNING("User %s (uid: %d) is not a member of the hugetlb_shm_group %s (gid: %d)!\n", pwd->pw_name, uid, grp->gr_name, gid);
	} else {
		printf("\n");
		printf("To make your hugetlb_shm_group settings persistent, add the following line to /etc/sysctl.conf:\n");
		printf("  vm.hugetlb_shm_group = %d\n", gid);
	}
}

void add_temp_swap(long page_size)
{
	char path[PATH_MAX];
	char file[PATH_MAX];
	char mkswap_cmd[PATH_MAX];
	FILE *f;
	char *buf;
	long swap_size;
	long pid;
	int ret;
	int num_pages;

	if (geteuid() != 0) {
		ERROR("Swap can only be manipulated by root\n");
		exit(EXIT_FAILURE);
	}

	pid = getpid();
	snprintf(path, PATH_MAX, "%s/swap/temp", MOUNT_DIR);
	snprintf(file, PATH_MAX, "%s/swapfile-%ld", path, pid);

	/* swapsize is 5 hugepages */
	if (opt_temp_swap == -1)
		num_pages = 5;
	else
		num_pages = opt_temp_swap;
	swap_size = num_pages * page_size;

	if (ensure_dir(path, S_IRWXU | S_IRGRP | S_IXGRP, 0, 0))
		exit(EXIT_FAILURE);

	if (opt_dry_run) {
		printf("dd bs=1024 count=%ld if=/dev/zero of=%s\n",
			swap_size / 1024, file);
		printf("mkswap %s\nswapon %s\n", file, file);
		return;
	}

	f = fopen(file, "wx");
	if (!f) {
		WARNING("Couldn't open %s: %s\n", file, strerror(errno));
		opt_temp_swap = 0;
		return;
	}

	buf = malloc(swap_size);
	memset(buf, 0, swap_size);
	fwrite(buf, sizeof(char), swap_size, f);
	free(buf);
	fclose(f);

	snprintf(mkswap_cmd, PATH_MAX, "mkswap %s", file);
	ret = system(mkswap_cmd);
	if (WIFSIGNALED(ret)) {
		WARNING("Call to mkswap failed\n");
		opt_temp_swap = 0;
		return;
	} else if (WIFEXITED(ret)) {
		ret = WEXITSTATUS(ret);
		if (ret) {
			WARNING("Call to mkswap failed\n");
			opt_temp_swap = 0;
			return;
		}
	}

	DEBUG("swapon %s\n", file);
	if (swapon(file, 0)) {
		WARNING("swapon on %s failed: %s\n", file, strerror(errno));
		opt_temp_swap = 0;
	}
}

void rem_temp_swap() {
	char file[PATH_MAX];
	long pid;

	pid = getpid();
	snprintf(file, PATH_MAX, "%s/swap/temp/swapfile-%ld", MOUNT_DIR, pid);

	if (opt_dry_run) {
		printf("swapoff %s\nrm -f %s\n", file, file);
		return;
	}

	if (swapoff(file))
		WARNING("swapoff on %s failed: %s\n", file, strerror(errno));
	remove(file);
	DEBUG("swapoff %s\n", file);
}

void add_ramdisk_swap(long page_size) {
	char ramdisk[PATH_MAX];
	char mkswap_cmd[PATH_MAX];
	int disk_num=0;
	int count = 0;
	long ramdisk_size;
	int ret;
	int fd;

	snprintf(ramdisk, PATH_MAX, "/dev/ram%i", disk_num);
	fd = open(ramdisk, O_RDONLY);
	ioctl(fd, BLKGETSIZE, &ramdisk_size);
	close(fd);

	ramdisk_size = ramdisk_size * 512;
	count = (page_size/ramdisk_size) + 1;

	if (count > 1) {
		INFO("Swap will be initialized on multiple ramdisks because\n\
		ramdisk size is less than huge page size. To avoid\n\
		this in the future, use kernel command line parameter\n\
		ramdisk_size=N, to set ramdisk size to N blocks.\n");
	}

	while (count > 0) {
		snprintf(ramdisk, PATH_MAX, "/dev/ram%i", disk_num);
		if (access(ramdisk, F_OK) != 0){
			break;
		}
		disk_num++;

		if (opt_dry_run) {
			printf("mkswap %s\nswapon %s\n", ramdisk, ramdisk);
		} else {
			snprintf(mkswap_cmd, PATH_MAX, "mkswap %s", ramdisk);
			ret = system(mkswap_cmd);
			if (WIFSIGNALED(ret)) {
				WARNING("Call to mkswap failed\n");
				continue;
			} else if (WIFEXITED(ret)) {
				ret = WEXITSTATUS(ret);
				if (ret) {
					WARNING("Call to mkswap failed\n");
					continue;
				}
			}
			DEBUG("swapon %s\n", ramdisk);
			if (swapon(ramdisk, 0)) {
				WARNING("swapon on %s failed: %s\n", ramdisk, strerror(errno));
				opt_temp_swap = 0;
				continue;
			}
		}
		count--;
		strcat(ramdisk_list, " ");
		strcat(ramdisk_list, ramdisk);
	}
}

void rem_ramdisk_swap(){
	char *ramdisk;
	char *iter = NULL;

	ramdisk = strtok_r(ramdisk_list, " ", &iter);
	while (ramdisk != NULL) {
		if (opt_dry_run) {
			printf("swapoff %s\n", ramdisk);
		} else {
			DEBUG("swapoff %s\n", ramdisk);
			if (swapoff(ramdisk)) {
				WARNING("swapoff on %s failed: %s\n", ramdisk, strerror(errno));
				continue;
			}
		}
		ramdisk = strtok_r(NULL, " ", &iter);
	}
}

void set_trans_opt(const char *file, const char *value)
{
	FILE *f;

	if (geteuid() != 0) {
                ERROR("Transparent huge page options can only be set by root\n");
                exit(EXIT_FAILURE);
        }

	if (opt_dry_run) {
		printf("echo '%s' > %s\n", value, file);
		return;
	}

	f = fopen(file, "w");
	if (!f) {
		ERROR("Couldn't open %s: %s\n", file, strerror(errno));
		return;
	}

	fprintf(f, "%s", value);
	fclose(f);
}

enum {
	POOL_MIN,
	POOL_MAX,
	POOL_BOTH,
};

static long value_adjust(char *adjust_str, long base, long page_size)
{
	long long adjust;
	char *iter;

	/* Convert and validate the adjust. */
	errno = 0;
	adjust = strtol(adjust_str, &iter, 0);
	/* Catch strtol errors and sizes that overflow the native word size */
	if (errno || adjust_str == iter) {
		if (errno == ERANGE)
			errno = EOVERFLOW;
		else
			errno = EINVAL;
		ERROR("%s: invalid adjustment\n", adjust_str);
		exit(EXIT_FAILURE);
	}

	/* size_to_smaller_unit() only works with positive values */
	if (adjust_str[0] == '-')
		adjust = -adjust;

	switch (*iter) {
	case 'G':
	case 'g':
		adjust = size_to_smaller_unit(adjust);
	case 'M':
	case 'm':
		adjust = size_to_smaller_unit(adjust);
	case 'K':
	case 'k':
		adjust = size_to_smaller_unit(adjust);
		adjust = adjust / page_size;
	}

	/* if previously negative, make negative again */
	if (adjust_str[0] == '-')
		adjust = -adjust;

	if (adjust_str[0] != '+' && adjust_str[0] != '-')
		base = 0;

	/* Ensure we neither go negative nor exceed LONG_MAX. */
	if (adjust < 0 && -adjust > base) {
		adjust = -base;
	}
	if (adjust > 0 && (base + adjust) < base) {
		adjust = LONG_MAX - base;
	}
	base += adjust;

	DEBUG("Returning page count of %ld\n", base);

	return base;
}


void pool_adjust(char *cmd, unsigned int counter)
{
	struct hpage_pool pools[MAX_POOLS];
	int pos;
	int cnt;

	char *iter = NULL;
	char *page_size_str = NULL;
	char *adjust_str = NULL;
	long page_size;

	unsigned long min;
	unsigned long min_orig;
	unsigned long max;
	unsigned long last_pool_value;

	/* Extract the pagesize and adjustment. */
	page_size_str = strtok_r(cmd, ":", &iter);
	if (page_size_str)
		adjust_str = strtok_r(NULL, ":", &iter);

	if (!page_size_str || !adjust_str) {
		ERROR("%s: invalid resize specification\n", cmd);
		exit(EXIT_FAILURE);
	}
	INFO("page_size<%s> adjust<%s> counter<%d>\n",
					page_size_str, adjust_str, counter);

	/* Convert and validate the page_size. */
	if (strcmp(page_size_str, "DEFAULT") == 0)
		page_size = kernel_default_hugepage_size();
	else
		page_size = parse_page_size(page_size_str);

	DEBUG("Working with page_size of %ld\n", page_size);

	cnt = hpool_sizes(pools, MAX_POOLS);
	if (cnt < 0) {
		ERROR("unable to obtain pools list");
		exit(EXIT_FAILURE);
	}
	for (pos = 0; cnt--; pos++) {
		if (pools[pos].pagesize == page_size)
			break;
	}
	if (cnt < 0) {
		ERROR("%s: unknown page size\n", page_size_str);
		exit(EXIT_FAILURE);
	}

	min_orig = min = pools[pos].minimum;
	max = pools[pos].maximum;

	if (counter == POOL_BOTH) {
		min = value_adjust(adjust_str, min, page_size);
		max = min;
	} else if (counter == POOL_MIN) {
		min = value_adjust(adjust_str, min, page_size);
		if (min > max)
			max = min;
	} else {
		max = value_adjust(adjust_str, max, page_size);
		if (max < min)
			min = max;
	}

	INFO("%ld, %ld -> %ld, %ld\n", pools[pos].minimum, pools[pos].maximum,
		min, max);

	if ((pools[pos].maximum - pools[pos].minimum) < (max - min)) {
		INFO("setting HUGEPAGES_OC to %ld\n", (max - min));
		set_huge_page_counter(page_size, HUGEPAGES_OC, (max - min));
	}

	if (opt_hard)
		cnt = 5;
	else
		cnt = -1;

	if (min > min_orig) {
		if (opt_temp_swap)
			add_temp_swap(page_size);
		if (opt_ramdisk_swap)
			add_ramdisk_swap(page_size);
		check_swap();
	}

	if (opt_obey_mempolicy && get_huge_page_counter(page_size,
				HUGEPAGES_TOTAL_MEMPOL) < 0) {
		opt_obey_mempolicy = 0;
		WARNING("Counter for NUMA huge page allocations is not found, continuing with normal pool adjustment\n");
	}

	INFO("setting HUGEPAGES_TOTAL%s to %ld\n",
		opt_obey_mempolicy ? "_MEMPOL" : "", min);
	set_huge_page_counter(page_size,
		opt_obey_mempolicy ? HUGEPAGES_TOTAL_MEMPOL : HUGEPAGES_TOTAL,
		min);
	get_pool_size(page_size, &pools[pos]);

	/* If we fail to make an allocation, retry if user requests */
	last_pool_value = pools[pos].minimum;
	while ((pools[pos].minimum != min) && (cnt > 0)) {
		/* Make note if progress is being made and sleep for IO */
		if (last_pool_value == pools[pos].minimum)
			cnt--;
		else
			cnt = 5;
		sleep(6);

		last_pool_value = pools[pos].minimum;
		INFO("Retrying allocation HUGEPAGES_TOTAL%s to %ld current %ld\n", opt_obey_mempolicy ? "_MEMPOL" : "", min, pools[pos].minimum);
		set_huge_page_counter(page_size,
			opt_obey_mempolicy ?
				HUGEPAGES_TOTAL_MEMPOL :
				HUGEPAGES_TOTAL,
			min);
		get_pool_size(page_size, &pools[pos]);
	}

	if (min > min_orig && !opt_swap_persist) {
		if (opt_temp_swap)
			rem_temp_swap();
		else if (opt_ramdisk_swap)
			rem_ramdisk_swap();
	}

	/*
	 * HUGEPAGES_TOTAL is not guarenteed to check to exactly the figure
	 * requested should there be insufficient pages.  Check the new
	 * value and adjust HUGEPAGES_OC accordingly.
	 */
	if (pools[pos].minimum != min) {
		WARNING("failed to set pool minimum to %ld became %ld\n",
			min, pools[pos].minimum);
		min = pools[pos].minimum;
	}
	if (pools[pos].maximum != max) {
		INFO("setting HUGEPAGES_OC to %ld\n", (max - min));
		set_huge_page_counter(page_size, HUGEPAGES_OC, (max - min));
	}
}

void page_sizes(int all)
{
	struct hpage_pool pools[MAX_POOLS];
	int pos;
	int cnt;

	cnt = hpool_sizes(pools, MAX_POOLS);
	if (cnt < 0) {
		ERROR("unable to obtain pools list");
		exit(EXIT_FAILURE);
	}
	qsort(pools, cnt, sizeof(pools[0]), cmpsizes);

	for (pos = 0; cnt--; pos++) {
		if (all || (pools[pos].maximum &&
		    hugetlbfs_find_path_for_size(pools[pos].pagesize)))
			printf("%ld\n", pools[pos].pagesize);
	}
}

void explain()
{
	show_mem();
	mounts_list_all();
	printf("\nHuge page pools:\n");
	pool_list();
	printf("\nHuge page sizes with configured pools:\n");
	page_sizes(0);
	check_minfreekbytes();
	check_shmmax();
	check_swap();
	check_user();
	printf("\nNote: Permanent swap space should be preferred when dynamic "
		"huge page pools are used.\n");
}

int main(int argc, char** argv)
{
	int ops;
	int has_hugepages = kernel_has_hugepages();

	char opts[] = "+hdv";
	char base[PATH_MAX];
	char *opt_min_adj[MAX_POOLS], *opt_max_adj[MAX_POOLS];
	char *opt_user_mounts = NULL, *opt_group_mounts = NULL;
	int opt_list_mounts = 0, opt_pool_list = 0, opt_create_mounts = 0;
	int opt_global_mounts = 0, opt_pgsizes = 0, opt_pgsizes_all = 0;
	int opt_explain = 0, minadj_count = 0, maxadj_count = 0;
	int opt_trans_always = 0, opt_trans_never = 0, opt_trans_madvise = 0;
	int opt_khuge_pages = 0, opt_khuge_scan = 0, opt_khuge_alloc = 0;
	int ret = 0, index = 0;
	char *khuge_pages = NULL, *khuge_alloc = NULL, *khuge_scan = NULL;
	gid_t opt_gid = 0;
	struct group *opt_grp = NULL;
	int group_invalid = 0;
	struct option long_opts[] = {
		{"help",       no_argument, NULL, 'h'},
		{"verbose",    required_argument, NULL, 'v' },

		{"list-all-mounts", no_argument, NULL, LONG_LIST_ALL_MOUNTS},
		{"pool-list", no_argument, NULL, LONG_POOL_LIST},
		{"pool-pages-min", required_argument, NULL, LONG_POOL_MIN_ADJ},
		{"pool-pages-max", required_argument, NULL, LONG_POOL_MAX_ADJ},
		{"obey-mempolicy", no_argument, NULL, LONG_POOL_MEMPOL},
		{"thp-always", no_argument, NULL, LONG_TRANS_ALWAYS},
		{"thp-madvise", no_argument, NULL, LONG_TRANS_MADVISE},
		{"thp-never", no_argument, NULL, LONG_TRANS_NEVER},
		{"thp-khugepaged-pages", required_argument, NULL, LONG_KHUGE_PAGES},
		{"thp-khugepaged-scan-sleep", required_argument, NULL, LONG_KHUGE_SCAN},
		{"thp-khugepaged-alloc-sleep", required_argument, NULL, LONG_KHUGE_ALLOC},
		{"set-recommended-min_free_kbytes", no_argument, NULL, LONG_SET_RECOMMENDED_MINFREEKBYTES},
		{"set-recommended-shmmax", no_argument, NULL, LONG_SET_RECOMMENDED_SHMMAX},
		{"set-shm-group", required_argument, NULL, LONG_SET_HUGETLB_SHM_GROUP},
		{"enable-zone-movable", no_argument, NULL, LONG_MOVABLE_ENABLE},
		{"disable-zone-movable", no_argument, NULL, LONG_MOVABLE_DISABLE},
		{"hard", no_argument, NULL, LONG_HARD},
		{"add-temp-swap", optional_argument, NULL, LONG_SWAP_DISK},
		{"add-ramdisk-swap", no_argument, NULL, LONG_SWAP_RAMDISK},
		{"persist", no_argument, NULL, LONG_SWAP_PERSIST},
		{"create-mounts", no_argument, NULL, LONG_CREATE_MOUNTS},
		{"create-user-mounts", required_argument, NULL, LONG_CREATE_USER_MOUNTS},
		{"create-group-mounts", required_argument, NULL, LONG_CREATE_GROUP_MOUNTS},
		{"create-global-mounts", no_argument, NULL, LONG_CREATE_GLOBAL_MOUNTS},

		{"max-size", required_argument, NULL, LONG_LIMIT_SIZE},
		{"max-inodes", required_argument, NULL, LONG_LIMIT_INODES},

		{"page-sizes", no_argument, NULL, LONG_PAGE_SIZES},
		{"page-sizes-all", no_argument, NULL, LONG_PAGE_AVAIL},
		{"dry-run", no_argument, NULL, 'd'},
		{"explain", no_argument, NULL, LONG_EXPLAIN},

		{0},
	};

	hugetlbfs_setup_debug();
	setup_mounts();
	verbose_init();

	ops = 0;
	while (ret != -1) {
		ret = getopt_long(argc, argv, opts, long_opts, &index);
		switch (ret) {
		case -1:
			break;

		case '?':
			print_usage();
			exit(EXIT_FAILURE);

		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);

		case 'v':
			verbose(optarg);
			continue;

		case 'd':
			opt_dry_run = 1;
			continue;

		default:
			/* All other commands require hugepage support. */
			if (! has_hugepages) {
				ERROR("kernel does not support huge pages\n");
				exit(EXIT_FAILURE);
			}
		}
		switch (ret) {
		case -1:
			break;

		case LONG_HARD:
			opt_hard = 1;
			continue;

		case LONG_SWAP_DISK:
			if (optarg)
				opt_temp_swap = atoi(optarg);
			else
				opt_temp_swap = -1;
			break;

		case LONG_SWAP_RAMDISK:
			opt_ramdisk_swap = 1;
			break;

		case LONG_SWAP_PERSIST:
			opt_swap_persist = 1;

		case LONG_LIST_ALL_MOUNTS:
			opt_list_mounts = 1;
			break;

		case LONG_POOL_LIST:
			opt_pool_list = 1;
			break;

		case LONG_POOL_MIN_ADJ:
			if (minadj_count == MAX_POOLS) {
				WARNING("Attempting to adjust an invalid "
					"pool or a pool multiple times, "
					"ignoring request: '%s'\n", optarg);
			} else {
				opt_min_adj[minadj_count++] = optarg;
			}
			break;

		case LONG_POOL_MEMPOL:
			opt_obey_mempolicy = 1;
			break;

		case LONG_TRANS_ALWAYS:
			opt_trans_always = 1;
			break;

		case LONG_TRANS_MADVISE:
			opt_trans_madvise = 1;
			break;

		case LONG_TRANS_NEVER:
			opt_trans_never = 1;
			break;

		case LONG_KHUGE_PAGES:
			opt_khuge_pages = 1;
			khuge_pages = optarg;
			break;

		case LONG_KHUGE_SCAN:
			opt_khuge_scan = 1;
			khuge_scan = optarg;
			break;

		case LONG_KHUGE_ALLOC:
			opt_khuge_alloc = 1;
			khuge_alloc = optarg;
			break;

		case LONG_POOL_MAX_ADJ:
			if (! kernel_has_overcommit()) {
				ERROR("kernel does not support overcommit, "
					"max cannot be adjusted\n");
				exit(EXIT_FAILURE);
			}

			if (maxadj_count == MAX_POOLS) {
				WARNING("Attempting to adjust an invalid "
					"pool or a pool multiple times, "
					"ignoring request: '%s'\n", optarg);
			} else {
				opt_max_adj[maxadj_count++] = optarg;
			}
                        break;

		case LONG_MOVABLE_ENABLE:
			opt_movable = 1;
			break;

		case LONG_SET_RECOMMENDED_MINFREEKBYTES:
			opt_set_recommended_minfreekbytes = 1;
			break;

		case LONG_SET_RECOMMENDED_SHMMAX:
			opt_set_recommended_shmmax = 1;
			break;

		case LONG_SET_HUGETLB_SHM_GROUP:
			opt_grp = getgrnam(optarg);
			if (!opt_grp) {
				opt_gid = atoi(optarg);
				if (opt_gid == 0 && strcmp(optarg, "0"))
					group_invalid = 1;
				opt_grp = getgrgid(opt_gid);
				if (!opt_grp)
					group_invalid = 1;
			} else {
				opt_gid = opt_grp->gr_gid;
			}
			if (group_invalid) {
				ERROR("Invalid group specification (%s)\n", optarg);
				exit(EXIT_FAILURE);
			}
			opt_set_hugetlb_shm_group = 1;
			break;

		case LONG_MOVABLE_DISABLE:
			opt_movable = 0;
			break;

		case LONG_CREATE_MOUNTS:
			opt_create_mounts = 1;
			break;

		case LONG_CREATE_USER_MOUNTS:
			opt_user_mounts = optarg;
			break;

		case LONG_CREATE_GROUP_MOUNTS:
			opt_group_mounts = optarg;
			break;

		case LONG_CREATE_GLOBAL_MOUNTS:
			opt_global_mounts = 1;
			break;

		case LONG_LIMIT_SIZE:
			/* Not a pagesize, but the conversions the same */
			opt_limit_mount_size = parse_page_size(optarg);
			if (!opt_limit_mount_size)
				WARNING("Mount max size specification 0, invalid or overflowed\n");
			break;

		case LONG_LIMIT_INODES:
			opt_limit_mount_inodes = atoi(optarg);
			break;

		case LONG_PAGE_SIZES:
			opt_pgsizes = 1;
			break;

		case LONG_PAGE_AVAIL:
			opt_pgsizes_all = 1;
			break;

		case LONG_EXPLAIN:
			opt_explain = 1;
			break;

		default:
			WARNING("unparsed option %08x\n", ret);
			ret = -1;
			break;
		}
		if (ret != -1)
			ops++;
	}

	verbose_expose();

	if (opt_list_mounts)
		mounts_list_all();

	if (opt_pool_list)
		pool_list();

	if (opt_movable != -1)
		setup_zone_movable(opt_movable);

	if (opt_trans_always)
		set_trans_opt(TRANS_ENABLE, ALWAYS);

	if (opt_trans_madvise)
		set_trans_opt(TRANS_ENABLE, MADVISE);

	if (opt_trans_never)
		set_trans_opt(TRANS_ENABLE, NEVER);

	if (opt_khuge_pages)
		set_trans_opt(KHUGE_SCAN_PAGES, khuge_pages);

	if (opt_khuge_alloc)
		set_trans_opt(KHUGE_ALLOC_SLEEP, khuge_alloc);

	if (opt_khuge_scan)
		set_trans_opt(KHUGE_SCAN_SLEEP, khuge_scan);

	if (opt_set_recommended_minfreekbytes)
		set_recommended_minfreekbytes();

	if (opt_set_recommended_shmmax)
		set_recommended_shmmax();

	if (opt_set_hugetlb_shm_group)
		set_hugetlb_shm_group(opt_gid, opt_grp->gr_name);

	while (--minadj_count >= 0) {
		if (! kernel_has_overcommit())
			pool_adjust(opt_min_adj[minadj_count], POOL_BOTH);
		else
			pool_adjust(opt_min_adj[minadj_count], POOL_MIN);
	}

	while (--maxadj_count >=0)
			pool_adjust(opt_max_adj[maxadj_count], POOL_MAX);

	if (opt_create_mounts) {
		snprintf(base, PATH_MAX, "%s", MOUNT_DIR);
		create_mounts(NULL, NULL, base, S_IRWXU | S_IRWXG);
	}


	if (opt_user_mounts != NULL) {
		snprintf(base, PATH_MAX, "%s/user", MOUNT_DIR);
		create_mounts(opt_user_mounts, NULL, base, S_IRWXU);
	}

	if (opt_group_mounts) {
		snprintf(base, PATH_MAX, "%s/group", MOUNT_DIR);
		create_mounts(NULL, opt_group_mounts, base, S_IRWXG);
	}

	if (opt_global_mounts) {
		snprintf(base, PATH_MAX, "%s/global", MOUNT_DIR);
		create_mounts(NULL, NULL, base, S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX );
	}

	if (opt_pgsizes)
		page_sizes(0);

	if (opt_pgsizes_all)
		page_sizes(1);

	if (opt_explain)
		explain();

	index = optind;

	if ((argc - index) != 0 || ops == 0) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

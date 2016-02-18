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
 * pagesize exposes the available and hardware supported page sizes on
 * the system.
 *
 * This program should be treated as an ABI for using libhugetlbfs.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define _GNU_SOURCE /* for getopt_long */
#include <unistd.h>
#include <getopt.h>

#define REPORT_UTIL "pagesize"
#include "libhugetlbfs_internal.h"
#include "hugetlbfs.h"

extern int errno;
extern int optind;
extern char *optarg;

#define OPTION(opts, text)	fprintf(stderr, " %-25s  %s\n", opts, text)
#define CONT(text) 		fprintf(stderr, " %-25s  %s\n", "", text)

void print_usage()
{
	fprintf(stderr, "pagesize [options] target\n");
	fprintf(stderr, "options:\n");

	OPTION("--help, -h", "Prints this message");

	OPTION("--all, -a", "show all supported page sizes");
	OPTION("--huge-only, -H", "show only huge page sizes");
}

static int cmpsizes(const void *p1, const void *p2)
{
	return *((long *)p1) > *((long *)p2);
}

#define MAX_PAGESIZES 32

int main(int argc, char** argv)
{
	int opt_all = 0;
	int opt_huge = 0;

	char opts[] = "+haH";
	int ret = 0, index = 0;
	struct option long_opts[] = {
		{"all",       no_argument, NULL, 'a'},
		{"huge-only", no_argument, NULL, 'H'},

		{0},
	};

	long pagesizes[MAX_PAGESIZES];
	int i;

	hugetlbfs_setup_debug();

	while (ret != -1) {
		ret = getopt_long(argc, argv, opts, long_opts, &index);
		switch (ret) {
		case '?':
			print_usage();
			exit(EXIT_FAILURE);

		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);

		case 'a':
			opt_all = 1;
			INFO("selecting all page sizes\n");
			break;

		case 'H':
			opt_huge = 1;
			opt_all = 1;
			INFO("selecting only huge page sizes\n");
			break;

		case -1:
			break;

		default:
			WARNING("unparsed option %08x\n", ret);
			ret = -1;
			break;
		}
	}
	index = optind;
	if ((argc - index) != 0) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (!opt_all) {
		pagesizes[0] = sysconf(_SC_PAGESIZE);
		ret = 1;
	} else if (opt_huge)
		ret = gethugepagesizes(pagesizes, MAX_PAGESIZES);
	else
		ret = getpagesizes(pagesizes, MAX_PAGESIZES);
	if (ret < 0) {
		ERROR("failed to get list of supported page sizes\n");
		exit(EXIT_FAILURE);
	}

	qsort(pagesizes, ret, sizeof(long), cmpsizes);
	for (i = 0; i < ret; i++) {
		printf("%ld\n", pagesizes[i]);
	}

	exit(EXIT_SUCCESS);
}

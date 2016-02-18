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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <signal.h>

#include <hugetlbfs.h>

#include "hugetests.h"

static sigjmp_buf sig_escape;
static void *sig_expected = MAP_FAILED;
static long hpage_size;

static void sig_handler(int signum, siginfo_t *si, void *uc)
{
	if (signum == SIGSEGV) {
		verbose_printf("SIGSEGV at %p (sig_expected=%p)\n", si->si_addr,
			       sig_expected);
		if (si->si_addr == sig_expected) {
			siglongjmp(sig_escape, 1);
		}
		FAIL("SIGSEGV somewhere unexpected");
	}
	FAIL("Unexpected signal %s", strsignal(signum));
}

static int test_read(void *p)
{
	volatile unsigned long *pl = p;
	unsigned long x;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SEGV */
		sig_expected = MAP_FAILED;
		return -1;
	}

	sig_expected = p;
	barrier();
	x = *pl;
	verbose_printf("Read back %lu\n", x);
	barrier();
	sig_expected = MAP_FAILED;
	/*
	 * gcc 5 complains about x not ever being used, the following
	 * statement is solely here to shut it up
	 */
	pl = (unsigned long *)x;

	return 0;
}

static int test_write(void *p, unsigned long val)
{
	volatile unsigned long *pl = p;
	unsigned long x;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SEGV */
		sig_expected = MAP_FAILED;
		return -1;
	}

	sig_expected = p;
	barrier();
	*pl = val;
	x = *pl;
	barrier();
	sig_expected = MAP_FAILED;

	return (x != val);
}

#define RANDOM_CONSTANT	0x1234ABCD

static void test_prot(void *p, int prot)
{
	int r, w;

	verbose_printf("Reading..");
	r = test_read(p);
	verbose_printf("%d\n", r);
	verbose_printf("Writing..");
	w = test_write(p, RANDOM_CONSTANT);
	verbose_printf("%d\n", w);

	if (prot & PROT_READ) {
		if (r != 0)
			FAIL("read failed on mmap(prot=%x)", prot);
	} else {
		if (r != -1)
			FAIL("read succeeded on mmap(prot=%x)", prot);
	}

	if (prot & PROT_WRITE) {
		switch (w) {
		case -1:
			FAIL("write failed on mmap(prot=%x)", prot);
			break;
		case 0:
			break;
		case 1:
			FAIL("write mismatch on mmap(prot=%x)", prot);
			break;
		default:
			TEST_BUG();
		}
	} else {
		switch (w) {
		case -1:
			break;
		case 0:
			FAIL("write succeeded on mmap(prot=%x)", prot);
			break;
		case 1:
			FAIL("write mismatch on mmap(prot=%x)", prot);
			break;
		default:
			TEST_BUG();
		}
	}
}

static void test_mprotect(int fd, char *testname,
			  unsigned long len1, int prot1,
			  unsigned long len2, int prot2)
{
	void *p;
	int err;

	verbose_printf("Testing %s\n", testname);
	verbose_printf("Mapping with prot=%x\n", prot1);
	p = mmap(NULL, len1, prot1, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("%s: mmap(prot=%x): %s", testname, prot1,
		     strerror(errno));

	test_prot(p, prot1);

	verbose_printf("mprotect()ing to prot=%x\n", prot2);
	err = mprotect(p, len2, prot2);
	if (err != 0)
		FAIL("%s: mprotect(prot=%x): %s", testname, prot2,
		     strerror(errno));

	test_prot(p, prot2);

	if (len2 < len1)
		test_prot(p + len2, prot1);

	munmap(p, len1);
}

int main(int argc, char *argv[])
{
	int err;
	int fd;
	void *p;

	test_init(argc, argv);

	struct sigaction sa = {
		.sa_sigaction = sig_handler,
		.sa_flags = SA_SIGINFO,
	};

	err = sigaction(SIGSEGV, &sa, NULL);
	if (err)
		FAIL("Can't install SIGSEGV handler: %s", strerror(errno));

	hpage_size = check_hugepagesize();

	fd = hugetlbfs_unlinked_fd();
	if (fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	verbose_printf("instantiating page\n");

	p = mmap(NULL, 2*hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));
	memset(p, 0, hpage_size);
	munmap(p, hpage_size);

	/* Basic protection change tests */
	test_mprotect(fd, "R->RW", hpage_size, PROT_READ,
		      hpage_size, PROT_READ|PROT_WRITE);
	test_mprotect(fd, "RW->R", hpage_size, PROT_READ|PROT_WRITE,
		      hpage_size, PROT_READ);

	/* Tests which require VMA splitting */
	test_mprotect(fd, "R->RW 1/2", 2*hpage_size, PROT_READ,
		      hpage_size, PROT_READ|PROT_WRITE);
	test_mprotect(fd, "RW->R 1/2", 2*hpage_size, PROT_READ|PROT_WRITE,
		      hpage_size, PROT_READ);

	/* PROT_NONE tests */
	test_mprotect(fd, "NONE->R", hpage_size, PROT_NONE,
		      hpage_size, PROT_READ);
	test_mprotect(fd, "NONE->RW", hpage_size, PROT_NONE,
		      hpage_size, PROT_READ|PROT_WRITE);

	PASS();
}

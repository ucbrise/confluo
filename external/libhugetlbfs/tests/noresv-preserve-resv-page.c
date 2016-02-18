/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2013 Joonsoo Kim, LG Electronics.
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
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <hugetlbfs.h>
#include "hugetests.h"

#define P "noresv-preserve-resv-page"
#define DESC \
	"* Test to preserve a reserved page against no-reserved maping.	   *\n"\
	"* If all hugepages are reserved, access to no-reserved shared	   *\n"\
	"* mapping cause a process die, instead of stealing a hugepage	   *\n"\
	"* which is reserved for other process				   *\n"

static sigjmp_buf sig_escape;
static void *sig_expected = MAP_FAILED;

static void sig_handler(int signum, siginfo_t *si, void *uc)
{
	if (signum == SIGBUS) {
		verbose_printf("SIGBUS at %p (sig_expected=%p)\n", si->si_addr,
			       sig_expected);
		if (si->si_addr == sig_expected) {
			siglongjmp(sig_escape, 1);
		}
		FAIL("SIGBUS somewhere unexpected");
	}
	FAIL("Unexpected signal %s", strsignal(signum));
}

static void test_write(void *p)
{
	volatile char *pl = p;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SIGBUS */
		PASS();
	}

	sig_expected = p;
	barrier();
	*pl = 's';
}

int main(int argc, char *argv[])
{
	long hpage_size;
	int nr_hugepages;
	int fd1, fd2, err;
	char *p, *q;
	struct sigaction sa = {
		.sa_sigaction = sig_handler,
		.sa_flags = SA_SIGINFO,
	};

	test_init(argc, argv);

	hpage_size = check_hugepagesize();
	nr_hugepages = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);

	fd1 = hugetlbfs_unlinked_fd();
	if (fd1 < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	fd2 = hugetlbfs_unlinked_fd();
	if (fd2 < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	err = sigaction(SIGBUS, &sa, NULL);
	if (err)
		FAIL("Can't install SIGBUS handler: %s", strerror(errno));

	p = mmap(NULL, hpage_size * nr_hugepages,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
	if (p == MAP_FAILED)
		FAIL("mmap() 1: %s", strerror(errno));

	verbose_printf("Reserve all hugepages %d\n", nr_hugepages);

	q = mmap(NULL, hpage_size,
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd2, 0);
	if (q == MAP_FAILED)
		FAIL("mmap() 2: %s", strerror(errno));

	verbose_printf("Write %c to %p to steal reserved page\n", *q, q);

	test_write(q);
	FAIL("Steal reserved page");
}

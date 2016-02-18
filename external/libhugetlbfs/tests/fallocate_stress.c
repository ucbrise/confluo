/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 20015 Mike Kravetz, Oracle Corporation
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
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/falloc.h>
#include <pthread.h>

#include <hugetlbfs.h>

#include "hugetests.h"

#define P "fallocate-stress"
#define DESC \
	"* Stress test fallocate.  This test starts three threads.  Thread *\n"\
	"* one will continually punch/fill holes via falloc.  Thread two   *\n"\
	"* will continually fault in those same pages.  Thread three will  *\n"\
	"* continually mmap/munmap that page range.                        *"

#define min(a,b) (((a) < (b)) ? (a) : (b))

#define MAX_PAGES_TO_USE 100

static int htlb_fd;
static long max_hpages;
static long hpage_size;

#define FALLOCATE_ITERATIONS 100000
static void *thread_fallocate(void *arg)
{
	int i, err;
	long tpage;

	for (i=0; i < FALLOCATE_ITERATIONS; i++) {
		tpage = ((long long)random()) % (max_hpages);
		err = fallocate(htlb_fd,
				FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
				tpage * hpage_size, hpage_size);
		if (err)
			FAIL("fallocate(): %s", strerror(errno));
		err = fallocate(htlb_fd, 0, tpage * hpage_size, hpage_size);
		if (err)
			FAIL("fallocate(FALLOC_FL_PUNCH_HOLE): %s",
				strerror(errno));
	}

	return NULL;
}

static void *fault_mmap_addr = NULL;

static void thread_fault_cleanup(void *arg)
{
	if (fault_mmap_addr)
		munmap(fault_mmap_addr, max_hpages * hpage_size);
}

static void *thread_fault(void *arg)
{
	long tpage;
	char foo;
	struct timespec ts;

	fault_mmap_addr = mmap(NULL, max_hpages * hpage_size,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				htlb_fd, 0);
	if (fault_mmap_addr == MAP_FAILED)
		FAIL("mmap(): %s", strerror(errno));

	pthread_cleanup_push(thread_fault_cleanup, NULL);

	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	while (1) {
		tpage = ((long long)random()) % (max_hpages);

		foo = *((char *)(fault_mmap_addr + (tpage * hpage_size)));
		*((char *)(fault_mmap_addr + (tpage * hpage_size))) = foo;

		nanosleep(&ts, NULL);	/* thread cancellation point */
	}

	pthread_cleanup_pop(1);

	return NULL;
}

static void *mmap_munmap_addr = NULL;

static void thread_mmap_munmap_cleanup(void *arg)
{
	if (mmap_munmap_addr)
		munmap(mmap_munmap_addr, max_hpages * hpage_size);
}

static void *thread_mmap_munmap(void *arg)
{
	int err;
	struct timespec ts;

	pthread_cleanup_push(thread_mmap_munmap_cleanup, NULL);

	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	while (1) {
		mmap_munmap_addr = mmap(NULL, max_hpages * hpage_size,
					PROT_READ | PROT_WRITE,
					MAP_SHARED, htlb_fd, 0);
		if (mmap_munmap_addr == MAP_FAILED)
			FAIL("mmap(): %s", strerror(errno));

		err = munmap(mmap_munmap_addr, max_hpages * hpage_size);
		if (err)
			FAIL("munmap(): %s", strerror(errno));
		mmap_munmap_addr = NULL;

		nanosleep(&ts, NULL);	/* thread cancellation point */
	}

	pthread_cleanup_pop(1);

	return NULL;
}

int main(int argc, char *argv[])
{
	long nr_hpages_free;
	int err;
	unsigned long free_before, free_after;
	unsigned long rsvd_before, rsvd_after;
	pthread_t falloc_th, fault_th, mmap_munmap_th;
	void *falloc_th_ret, *fault_th_ret, *mmap_munmap_th_ret;

	test_init(argc, argv);

	srandom((int)getpid() * time(NULL));
	hpage_size = check_hugepagesize();
	nr_hpages_free = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	max_hpages = min(nr_hpages_free, MAX_PAGES_TO_USE);

	htlb_fd = hugetlbfs_unlinked_fd();
	if (htlb_fd < 0)
		FAIL("hugetlbfs_unlinked_fd()");

	free_before = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	rsvd_before = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);

	/* First preallocate file with max_hpages pages */
	err = fallocate(htlb_fd, 0, 0, hpage_size * max_hpages);
	if (err) {
		if (errno == EOPNOTSUPP)
			IRRELEVANT();
		if (err)
			FAIL("fallocate(): %s", strerror(errno));
	}

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	if (free_before - free_after != max_hpages)
		FAIL("fallocate did not preallocate %ld huge pages\n",
							max_hpages);

	err = pthread_create(&falloc_th, NULL, thread_fallocate, NULL);
	if (err != 0)
		FAIL("pthread_create(): %s\n", strerror(errno));
	err = pthread_create(&fault_th, NULL, thread_fault, NULL);
	if (err != 0)
		FAIL("pthread_create(): %s\n", strerror(errno));
	err = pthread_create(&mmap_munmap_th, NULL, thread_mmap_munmap, NULL);
	if (err != 0)
		FAIL("pthread_create(): %s\n", strerror(errno));

	err = pthread_join(falloc_th, &falloc_th_ret);
	if (err != 0)
		FAIL("pthread_join(): %s\n", strerror(errno));
	if (falloc_th_ret)
		FAIL("thread_fallocate unexpected exit code\n");

	err = pthread_cancel(fault_th);
	if (err != 0)
		FAIL("pthread_cancel(): %s\n", strerror(errno));
	err = pthread_join(fault_th, &fault_th_ret);
	if (err != 0)
		FAIL("pthread_join(): %s\n", strerror(errno));

	err = pthread_cancel(mmap_munmap_th);
	if (err != 0)
		FAIL("pthread_cancel(): %s\n", strerror(errno));
	err = pthread_join(mmap_munmap_th, &mmap_munmap_th_ret);
	if (err != 0)
		FAIL("pthread_join(): %s\n", strerror(errno));

	if (close(htlb_fd))
		FAIL("close(): %s", strerror(errno));

	free_after = get_huge_page_counter(hpage_size, HUGEPAGES_FREE);
	rsvd_after = get_huge_page_counter(hpage_size, HUGEPAGES_RSVD);
	if (free_after != free_before || rsvd_after != rsvd_before)
		FAIL("free or reserve counts not correct after fallocate stress testing\n");

	PASS();
}

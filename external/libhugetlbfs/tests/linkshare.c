/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2006 Nishanth Aravamudan, IBM Corporation
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
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "hugetests.h"

#define BLOCK_SIZE	16384
#define CONST	0xdeadbeef
#define SHM_KEY 0xdeadcab
#define NUM_CHILDREN	2

#define BIG_INIT	{ \
	[0] = CONST, [17] = CONST, [BLOCK_SIZE-1] = CONST, \
}
static int small_data = 1;
static int big_data[BLOCK_SIZE] = BIG_INIT;

static int small_bss;
static int big_bss[BLOCK_SIZE];

const int small_const = CONST;
const int big_const[BLOCK_SIZE] = BIG_INIT;

static int static_func(int x)
{
	return x;
}

int global_func(int x)
{
	return x;
}

static struct test_entry {
	const char *name;
	void *data;
	int size;
	char linkchar;
	int writable, execable;
	int is_huge;
} testtab[] = {
#define RWENT(name, linkchar)	{ #name, &name, sizeof(name), linkchar, 1, 0, }
#define ROENT(name, linkchar)	{ #name, (void *)&name, sizeof(name), linkchar, 0, 0, }
#define RXENT(name, linkchar)	{ #name, &name, sizeof(name), linkchar, 0, 1, }
	RWENT(small_data, 'D'),
	RWENT(big_data, 'D'),
	RWENT(small_bss, 'B'),
	RWENT(big_bss, 'B'),
	ROENT(small_const, 'T'),
	ROENT(big_const, 'T'),
	RXENT(static_func, 'T'),
	RXENT(global_func, 'T'),
};

#define NUM_TESTS	(sizeof(testtab) / sizeof(testtab[0]))

static int sharing;
static int elfmap_off;
static int shmid;
static ino_t *shm;

static char link_string[32];

static void get_link_string(const char *argv0)
{
	const char *p, *q;

	/* Find program basename */
	p = strrchr(argv0, '/');
	if (p)
		p++;
	else
		p = argv0;

	if (*p != 'x')
		return; /* just a plain ordinary link */

	q = strchr(p, '.');
	if (!q)
		/* ERROR? */
		return;

	memcpy(link_string, p, q-p);
}

static ino_t do_test(struct test_entry *te)
{
	int i;
	volatile int *p = te->data;

	if (te->writable) {
		for (i = 0; i < (te->size / sizeof(*p)); i++)
			p[i] = CONST ^ i;

		barrier();

		for (i = 0; i < (te->size / sizeof(*p)); i++) {
			if (p[i] != (CONST ^ i)) {
				verbose_printf("mismatch on %s", te->name);
				exit(RC_FAIL);
			}
		}
	} else if (te->execable) {
		int (*pf)(int) = te->data;

		if ((*pf)(CONST) != CONST) {
			verbose_printf("%s returns incorrect results", te->name);
			exit(RC_FAIL);
		}
	} else {
		/* Otherwise just read touch it */
		for (i = 0; i < (te->size / sizeof(*p)); i++)
			p[i];
	}

	te->is_huge = (test_addr_huge(te->data) == 1);

	return get_addr_inode(te->data);
}

static void parse_env(void)
{
	char *env;

	env = getenv("HUGETLB_ELFMAP");
	if (env && (strcasecmp(env, "no") == 0)) {
		verbose_printf("Segment remapping disabled\n");
		elfmap_off = 1;
	} else {
		env = getenv("HUGETLB_SHARE");
		if (env)
			sharing = atoi(env);
		verbose_printf("Segment remapping enabled, "
				"sharing = %d\n", sharing);
	}
}

static pid_t spawn_child(char *self, int index)
{
	int ret;
	char execarg1[5];

	ret = snprintf(execarg1, 5, "%d", index);
	if (ret < 0)
		FAIL("snprintf failed: %s", strerror(errno));

	ret = fork();
	if (ret) {
		if (ret < 0) {
			shmctl(shmid, IPC_RMID, NULL);
			shmdt(shm);
			FAIL("fork failed: %s",
					strerror(errno));
		}
	} else {
		ret = execlp(self, self, execarg1, NULL);
		if (ret) {
			shmctl(shmid, IPC_RMID, NULL);
			shmdt(shm);
			FAIL("execl(%s, %s, %s failed: %s",
					self, self, execarg1,
					strerror(errno));
		}
	}

	return ret;
}

static int child_process(char *self, int index)
{
	int i;
	ino_t ino;

	get_link_string(self);

	shmid = shmget(SHM_KEY, NUM_CHILDREN * NUM_TESTS *
						sizeof(ino_t), 0666);
	if (shmid < 0) {
		verbose_printf("Child's shmget failed: %s", strerror(errno));
		exit(RC_FAIL);
	}

	shm = shmat(shmid, NULL, 0);
	if (shm == (void *)-1) {
		verbose_printf("Child's shmat failed: %s", strerror(errno));
		exit(RC_FAIL);
	}

	for (i = 0; i < NUM_TESTS; i++) {
		if (!test_addr_huge(testtab + i)) {
			/* don't care about non-huge addresses */
			shm[index * NUM_TESTS + i] = 0;
		} else {
			ino = do_test(testtab + i);
			if ((int)ino < 0) {
				shmdt(shm);
				exit(RC_FAIL);
			}
			shm[index * NUM_TESTS + i] = ino;
		}
	}
	shmdt(shm);
	return 0;
}

static void verify_inodes()
{
	int i, j;

	for (i = 0; i < NUM_TESTS; i++) {
		ino_t base = shm[i];
		for (j = 1; j < NUM_CHILDREN; j++) {
			ino_t comp = shm[j * NUM_TESTS + i];
			if (base != comp) {
				/*
				 * we care if we mismatch if
				 * sharing only read-only
				 * segments and this is one
				 */
				if (sharing == 1 && testtab[i].writable == 0) {
					shmctl(shmid, IPC_RMID, NULL);
					shmdt(shm);
					FAIL("Inodes do not match "
							"(%u != %u)",
							(int)base, (int)comp);
				}
			} else {
				/*
				 * we care if we match if
				 * a) not remapping or
				 * b) not sharing or
				 * c) sharing only read-only
				 * segments and this is not one
				 * BUT only if the inode is not
				 * 0 (don't care about the file)
				 */
				if (base == 0)
					continue;

				if (elfmap_off == 1 || sharing == 0 ||
				   (sharing == 1 && testtab[i].writable == 1)) {
					shmctl(shmid, IPC_RMID, NULL);
					shmdt(shm);
					if (sharing == 1 && testtab[i].writable == 1)
						verbose_printf("Incorrectly sharing a writable segment...\n");
					FAIL("Inodes match, but we should not be "
						"sharing this segment (%d == %d)",
						(int)base, (int)comp);
				}
			}
		}
	}
}

static void sigsegv_handler(int signum, siginfo_t *si, void *context)
{
	FAIL("Segmentation fault in parent at address %p", si->si_addr);
}

int main(int argc, char *argv[], char *envp[])
{
	test_init(argc, argv);

	if (argc == 1) {
		/*
		 * first process
		 */
		pid_t children_pids[NUM_CHILDREN];
		int ret, i;
		int status;
		/*
		 * We catch children's segfaults via waitpid's status,
		 * but this is to catch the parent itself segfaulting.
		 * This can happen, for instance, if an old (bad)
		 * segment file is left lying around in the hugetlbfs
		 * mountpoint
		 */
		struct sigaction sa_seg = {
			.sa_sigaction = sigsegv_handler,
			.sa_flags = SA_SIGINFO,
		};

		parse_env();

		ret = sigaction(SIGSEGV, &sa_seg, NULL);
		if (ret < 0)
			FAIL("Installing SIGSEGV handler failed: %s",
							strerror(errno));

		shmid = shmget(SHM_KEY, NUM_CHILDREN * NUM_TESTS *
					sizeof(ino_t), IPC_CREAT | IPC_EXCL |
					0666);
		if (shmid < 0)
			FAIL("Parent's shmget failed: %s", strerror(errno));

		shm = shmat(shmid, NULL, 0);
		if (shm == (void *)-1)
			FAIL("Parent's shmat failed: %s", strerror(errno));

		for (i = 0; i < NUM_CHILDREN; i++)
			children_pids[i] = spawn_child(argv[0], i);

		for (i = 0; i < NUM_CHILDREN; i++) {
			ret = waitpid(children_pids[i], &status, 0);
			if (ret < 0) {
				shmctl(shmid, IPC_RMID, NULL);
				shmdt(shm);
				FAIL("waitpid failed: %s", strerror(errno));
			}

			if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
				shmctl(shmid, IPC_RMID, NULL);
				shmdt(shm);
				FAIL("Child %d exited with non-zero status: %d",
							i + 1, WEXITSTATUS(status));
			}

			if (WIFSIGNALED(status)) {
				shmctl(shmid, IPC_RMID, NULL);
				shmdt(shm);
				FAIL("Child %d killed by signal: %s", i + 1,
							strsignal(WTERMSIG(status)));
			}
		}

		verify_inodes();

		shmctl(shmid, IPC_RMID, NULL);
		shmdt(shm);
		PASS();
	} else {
		if (argc == 2) {
			/*
			 * child process
			 * arg1 = index + 1 into shared memory array
			 */
			child_process(argv[0], atoi(argv[1]));
		} else {
			FAIL("Invalid arguments\n");
		}
	}

	return 0;
}

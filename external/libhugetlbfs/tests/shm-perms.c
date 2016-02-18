/*
 * libhugetlbfs - Easy use of Linux hugepages
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
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <hugetlbfs.h>
#include "hugetests.h"

#define P "shm-perms"
#define DESC \
	"* Test shared memory behavior when multiple threads are attached  *\n"\
	"* to a segment with different permissions.  A segment is created  *\n"\
	"* and children attach read-only to check reservation accounting.  *"

#define SEGMENT_SIZE	((size_t)0x4000000)
#define SEGMENT_KEY	0x82ba15ff
#define STRIDE		0x200000

static int global_shmid = -1;
void *shm_addr = NULL;

void cleanup(void)
{
	remove_shmid(global_shmid);
}

int attach_segment(size_t segsize, int shmflags, int shmperms)
{
	int shmid;

	/* Create/get large segment */
	shmid = shmget(SEGMENT_KEY, segsize, shmflags);
	if (shmid == -1) {
		perror("shmget(SEGMENT)");
		cleanup();
		exit(EXIT_FAILURE);
	}

	/* Attach large segment */
	if ( (shm_addr = shmat(shmid, shm_addr, shmperms)) == (void *)-1) {
		perror("shmat(SEGMENT)");
		cleanup();
		exit(EXIT_FAILURE);
	}

	global_shmid = shmid;
	return shmid;
}

int main(int argc, char **argv)
{
	char *p;
	pid_t *wait_list;
	int i, iterations;
	long hpage_size = check_hugepagesize();
	long total_hpages = get_huge_page_counter(hpage_size, HUGEPAGES_TOTAL);

	/* Setup */
	test_init(argc, argv);
	check_hugetlb_shm_group();
	if (hpage_size > SEGMENT_SIZE)
		CONFIG("Page size is too large for configured SEGMENT_SIZE\n");
	check_free_huge_pages(SEGMENT_SIZE / hpage_size);

	iterations = (total_hpages * hpage_size) / SEGMENT_SIZE + 1;
	verbose_printf("iterations = %d\n", iterations);

	wait_list = malloc(sizeof(pid_t) * iterations);
	if (wait_list == NULL)
		FAIL("Failed to allocate wait_list");

	/* Create, attach and part init segment */
	attach_segment(SEGMENT_SIZE, IPC_CREAT|SHM_HUGETLB|0640, 0);
	p = (char *)shm_addr;
	for (i = 0; i < 4; i++, p += STRIDE)
		memset(p, 0x55, STRIDE);

	/* Detach segment */
	if (shmdt(shm_addr) != 0)
		FAIL("shmdt(SEGMENT)");

	/* Create children to reattach read-only */
	for (i = 0; i < iterations; i++) {
		pid_t pid;
		pid = fork();
		if (pid == -1)
			FAIL("fork");

		if (pid) {
			wait_list[i] = pid;
		} else {
			attach_segment(0, 0, SHM_RDONLY);
			if (shmdt(shm_addr) != 0) {
				perror("shmdt(SEGMENT)");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}
	}

	/* Wait for all children to exit */
	for (i = 0; i < iterations; i++) {
		int status;
		if (waitpid(wait_list[i], &status, 0) == -1)
			FAIL("waitpid");
		if (status != EXIT_SUCCESS)
			FAIL("Child exited with failure");
	}

	PASS();
}

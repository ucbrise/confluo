/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2005-2008 David Gibson & Adam Litke, IBM Corporation.
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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <elf.h>
#include <link.h>

#include "hugetests.h"

#define BLOCK_SIZE	16384
#define CONST		0xdeadbeef
#define RETURN_ADDRESS	0x0

#define BIG_INIT	{ \
	[0] = CONST, [17] = CONST, [BLOCK_SIZE-1] = CONST, \
}
static int small_data = 1;
static int big_data[BLOCK_SIZE] = BIG_INIT;

static int small_bss;
static int big_bss[BLOCK_SIZE];

const int small_const = CONST;
const int big_const[BLOCK_SIZE] = BIG_INIT;

/*
 * Turn function pointer into address from .text.
 *
 * On some ABIs function pointer may not refer to .text section. For example
 * on powerPC 64-bit ABI, function pointer may refer to call stub from
 * .opd section.
 *
 * This function expects that parameter data is a function pointer of type:
 * long f(long), and when called with special parameter, it returns an address
 * corresponding to actual code of the function. Current implementation relies
 * on gcc's __builtin_return_address, see get_pc() below.
 */
static inline void *get_text_addr(void *data)
{
	long (*gettext)(long) = data;

	return (void *)gettext(RETURN_ADDRESS);
}

static void __attribute__ ((noinline)) *get_pc(void)
{
#if defined(__s390__) && __WORDSIZE == 32
	/* taken from sysdeps/unix/sysv/linux/s390/s390-32/profil-counter.h
	 * 31-bit s390 pointers don't use the 32th bit, however integers do,
	 * so wrap the value around at 31 bits */
	return (void *)
		((unsigned long) __builtin_return_address(0) & 0x7fffffffUL);
#else
	return __builtin_return_address(0);
#endif
}

static long static_func(long x)
{
	if (x == RETURN_ADDRESS)
		return (long)get_pc();
	return x;
}

long global_func(long x)
{
	if (x == RETURN_ADDRESS)
		return (long)get_pc();
	return x;
}

static struct test_entry {
	const char *name;
	void *data;
	int size;
	int writable;
	int execable;
	int is_huge;
} testtab[] = {
#define ENT(entry_name, exec) { \
	.name = #entry_name, \
	.data = (void *)&entry_name, \
	.size = sizeof(entry_name), \
	.writable = 0, \
	.execable = exec }

	ENT(small_data, 0),
	ENT(big_data, 0),
	ENT(small_bss, 0),
	ENT(big_bss, 0),
	ENT(small_const, 0),
	ENT(big_const, 0),
	ENT(static_func, 1),
	ENT(global_func, 1),
};


#define NUM_TESTS	(sizeof(testtab) / sizeof(testtab[0]))

static
int parse_elf(struct dl_phdr_info *info, size_t size, void *data)
{
	int i;
	unsigned long text_end, data_start;
	long *min_align = (long *)data;
	long actual_align;

	text_end = data_start = 0;
	for (i = 0; i < info->dlpi_phnum; i++) {
		if (info->dlpi_phdr[i].p_type != PT_LOAD)
			continue;

		if (info->dlpi_phdr[i].p_flags & PF_X)
			text_end = info->dlpi_phdr[i].p_vaddr +
					info->dlpi_phdr[i].p_memsz;
		else if (info->dlpi_phdr[i].p_flags & PF_W)
			data_start = info->dlpi_phdr[i].p_vaddr;

		if (text_end && data_start)
			break;
	}

	actual_align = (data_start - text_end) / 1024;
	if (actual_align < *min_align)
		FAIL("Binary not suitably aligned");

	return 1;
}

static void check_if_writable(struct test_entry *te)
{
	int pid, ret, status;

	pid = fork();
	if (pid < 0)
		FAIL("fork: %s", strerror(errno));
	else if (pid == 0) {
		void *data;

		if (te->execable)
			data = get_text_addr(te->data);
		else
			data = te->data;

		(*(char *)data) = 0;
		exit (0);
	} else {
		ret = waitpid(pid, &status, 0);
		if (ret < 0)
			FAIL("waitpid(): %s", strerror(errno));
		if (WIFSIGNALED(status))
			te->writable = 0;
		else
			te->writable = 1;
	}
}

static void do_test(struct test_entry *te)
{
	int i;
	void *data = te->data;

	check_if_writable(te);
	verbose_printf("entry: %s, data: %p, writable: %d\n",
		te->name, data, te->writable);

	if (te->writable) {
		volatile int *p = data;

		for (i = 0; i < (te->size / sizeof(*p)); i++)
			p[i] = CONST ^ i;

		barrier();

		for (i = 0; i < (te->size / sizeof(*p)); i++)
			if (p[i] != (CONST ^ i))
				FAIL("mismatch on %s", te->name);
	} else if (te->execable) {
		long (*pf)(long) = data;

		data = get_text_addr(data);

		if ((*pf)(CONST) != CONST)
			FAIL("%s returns incorrect results", te->name);
	} else {
		/* Otherwise just read touch it */
		volatile int *p = data;

		for (i = 0; i < (te->size / sizeof(*p)); i++)
			p[i];
	}

	te->is_huge = (test_addr_huge(data) == 1);
	verbose_printf("entry: %s, data: %p, is_huge: %d\n",
		te->name, data, te->is_huge);
}

int main(int argc, char *argv[])
{
	int i;
	char *env;
	int elfmap_readonly, elfmap_writable;
	long hpage_size = gethugepagesize() / 1024;

	test_init(argc, argv);

	/* Test that the binary has been aligned enough by the linker */
	if ((argc > 1) && !strcmp("--test-alignment", argv[1]))
		dl_iterate_phdr(parse_elf, &hpage_size);

	env = getenv("HUGETLB_ELFMAP");
	verbose_printf("HUGETLB_ELFMAP=%s\n", env);

	elfmap_readonly = env && strchr(env, 'R');
	elfmap_writable = env && strchr(env, 'W');

	for (i = 0; i < NUM_TESTS; i++) {
		do_test(testtab + i);
	}

	verbose_printf("Hugepages used for:");
	for (i = 0; i < NUM_TESTS; i++)
		if (testtab[i].is_huge)
			verbose_printf(" %s", testtab[i].name);
	verbose_printf("\n");

	for (i = 0; i < NUM_TESTS; i++) {
		if (testtab[i].writable) {
			if (elfmap_writable && !testtab[i].is_huge)
				FAIL("%s is not hugepage", testtab[i].name);
			if (!elfmap_writable && testtab[i].is_huge)
				FAIL("%s is hugepage", testtab[i].name);
		} else if (!testtab[i].writable) {
			if (elfmap_readonly && !testtab[i].is_huge)
				FAIL("%s is not hugepage", testtab[i].name);
			if (!elfmap_readonly && testtab[i].is_huge)
				FAIL("%s is hugepage", testtab[i].name);
		}
	}
	PASS();
}

/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 Adam Litke, IBM Corporation.
 *
 * This program is free software; you can redistribute it and/or
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
#include <fcntl.h>
#include <elf.h>
#include <link.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>

/*
 * Eventually we plan to use the libhugetlbfs reporting facility,
 * but until that is possible, redefine a simpler version here.
 */
#define REPORT(level, prefix, format, ...) \
	do { \
		fprintf(stderr, "hugeedit: " prefix ": " format, \
			##__VA_ARGS__); \
	} while (0)

#include "libhugetlbfs_internal.h"

/*
 * All MAP_* options are tagged with MAP_BASE to differentiate them as options
 * in the options parser.  This must be removed before they are compared.
 */
#define MAP_BASE	0x1000
#define MAP_DISABLE	0x0001
#define MAP_TEXT	0x0002
#define MAP_DATA	0x0004

#define PF_LINUX_HUGETLB	0x100000
extern int optind;
extern char *optarg;

#define OPTION(opts, text)	fprintf(stderr, " %-25s  %s\n", opts, text)
#define CONT(text)		fprintf(stderr, " %-25s  %s\n", "", text)

void print_usage()
{
	fprintf(stderr, "hugeedit [options] target\n");
	fprintf(stderr, "options:\n");
	OPTION("--text", "Remap program text into huge pages by default");
	OPTION("--data", "Remap program data into huge pages by default");
	OPTION("--disable", "Remap no segments into huge pages by default");
	OPTION("--help, -h", "Print this usage information");
}

int check_elf_wordsize(void *ehdr)
{
	char *e_ident = (char *) ehdr;

	if (strncmp(e_ident, ELFMAG, SELFMAG)) {
		ERROR("Not a valid ELF executable\n");
		exit(EXIT_FAILURE);
	}

	switch (e_ident[EI_CLASS]) {
		case ELFCLASS32:
		case ELFCLASS64:
			return e_ident[EI_CLASS];
		default:
			ERROR("Can not determine word size\n");
			exit(EXIT_FAILURE);
	}
}

/*
 * We need to map enough of the binary so that we can access the ELF header and
 * all of the program headers.  This function takes a pointer to the first page
 * of ELF headers which is guaranteed to be enough data to determine if we need
 * to map more of the binary.  Use mremap to enlarge the mapping if needed.
 *
 * void **elf - may be updated with a new address if mremap moved it
 * unsigned long *size - may be updated with the new mapping size
 */
#define elf_ph_end_offset(e) ((e)->e_phoff + (e)->e_phentsize * (e)->e_phnum)
void check_remap_elf(void **elf, unsigned long *size, int wordsize)
{
	unsigned long newsize;
	int pagesize = getpagesize();

	if (wordsize == ELFCLASS32) {
		Elf32_Ehdr *ehdr = *(Elf32_Ehdr **) elf;
		newsize = elf_ph_end_offset(ehdr);
	} else {
		Elf64_Ehdr *ehdr = *(Elf64_Ehdr **) elf;
		newsize = elf_ph_end_offset(ehdr);
	}
	newsize = ALIGN_UP(newsize, pagesize);

	if (newsize > *size) {
		*size = newsize;
		*elf = mremap(*elf, *size, newsize, MREMAP_MAYMOVE);
		if (*elf == MAP_FAILED) {
			ERROR("Remapping failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

#define is_text(p) ((((p)->p_flags & (PF_R|PF_W|PF_X)) == (PF_R|PF_X)) && \
			((p)->p_memsz == (p)->p_filesz))
#define is_data(p) (((p)->p_flags & (PF_R|PF_W|PF_X)) == (PF_R|PF_W))

#define update_phdrs(_BITS_) 						\
void update_phdrs##_BITS_(Elf##_BITS_##_Ehdr *ehdr, int remap_opts)	\
{									\
	int i;								\
	Elf##_BITS_##_Phdr *phdr;					\
	unsigned long long start, end;					\
									\
	phdr = (Elf##_BITS_##_Phdr *)((char *)ehdr + ehdr->e_phoff);	\
	for (i = 0; i < ehdr->e_phnum; i++) {				\
		if (phdr[i].p_type != PT_LOAD)				\
			continue;					\
		if (remap_opts)						\
			phdr[i].p_flags &= ~PF_LINUX_HUGETLB;		\
		if ((remap_opts & MAP_TEXT) && is_text(&phdr[i]))	\
			phdr[i].p_flags |= PF_LINUX_HUGETLB;		\
		if ((remap_opts & MAP_DATA) && is_data(&phdr[i]))	\
			phdr[i].p_flags |= PF_LINUX_HUGETLB;		\
		start = (unsigned long long) phdr[i].p_vaddr;		\
		end = start + phdr[i].p_memsz;				\
		printf("Segment %i 0x%llx - 0x%llx (%s%s) default is "	\
			"%s pages\n", i, start, end,			\
			is_text(&phdr[i]) ? "TEXT" : "",		\
			is_data(&phdr[i]) ? "DATA" : "",		\
			(phdr[i].p_flags & PF_LINUX_HUGETLB) ?		\
			"HUGE" : "BASE");				\
	}								\
}
update_phdrs(32)
update_phdrs(64)

int main(int argc, char ** argv)
{
	char opts[] = "+h";
	struct option long_opts[] = {
		{"help",	no_argument, NULL, 'h'},
		{"disable",	no_argument, NULL, MAP_BASE|MAP_DISABLE},
		{"text",	no_argument, NULL, MAP_BASE|MAP_TEXT},
		{"data",	no_argument, NULL, MAP_BASE|MAP_DATA},
		{0},
	};
	int ret = 0, index = 0, remap_opts = 0;
	int fd;
	const char *target;
	void *ehdr;
	unsigned long mapsize = getpagesize();
	int target_wordsize;

	while (ret != -1) {
		ret = getopt_long(argc, argv, opts, long_opts, &index);
		if (ret > 0 && (ret & MAP_BASE)) {
			remap_opts |= ret;
			continue;
		}
		switch (ret) {
		case '?':
			print_usage();
			exit(EXIT_FAILURE);
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);

		default:
			ret = -1;
			break;
		}
	}
	index = optind;
	remap_opts &= ~MAP_BASE;
	if (remap_opts & MAP_DISABLE && remap_opts != MAP_DISABLE) {
		ERROR("--disable is not compatible with --text or --data\n");
		exit(EXIT_FAILURE);
	}

	if ((argc - index) != 1) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	target = argv[index];

	/* We don't need write access unless we plan to alter the binary */
	fd = open(target, (remap_opts ? O_RDWR : O_RDONLY));
	if (fd < 0) {
		ERROR("Opening %s failed: %s\n", target, strerror(errno));
		exit(EXIT_FAILURE);
	}

	ehdr = mmap(NULL, mapsize, PROT_READ | (remap_opts ? PROT_WRITE : 0),
						MAP_SHARED, fd, 0);
	if (ehdr == MAP_FAILED) {
		ERROR("Mapping %s failed: %s\n", target, strerror(errno));
		exit(EXIT_FAILURE);
	}

	target_wordsize = check_elf_wordsize(ehdr);
	check_remap_elf(&ehdr, &mapsize, target_wordsize);
	if (target_wordsize == ELFCLASS64)
		update_phdrs64((Elf64_Ehdr *) ehdr, remap_opts);
	else
		update_phdrs32((Elf32_Ehdr *) ehdr, remap_opts);

	if (munmap(ehdr, mapsize) != 0) {
		ERROR("Unmapping %s failed: %s\n", target, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (close(fd) != 0) {
		ERROR("Final close of %s failed: %s -- possible data loss!\n",
			target, strerror(errno));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

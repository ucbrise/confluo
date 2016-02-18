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
#include <link.h>

#include "hugetests.h"

static int parse_phdrs(struct dl_phdr_info *info, size_t size, void *data)
{
	int i;
	/* This should only be iterated once - we assume that the
	 * first iteration is the phdrs for the main executable */

	for (i = 0; i < info->dlpi_phnum; i++) {
		const ElfW(Phdr) *phdr = &info->dlpi_phdr[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		verbose_printf("PHDR %d: filesz = 0x%lx, memsz = 0x%lx\n",
			       i, (unsigned long)phdr->p_filesz,
			       (unsigned long)phdr->p_memsz);
		if (phdr->p_filesz == 0)
			PASS();
	}

	return 1;
}

int main(int argc, char *argv[])
{
	test_init(argc, argv);

	/* If we're even able to load, that's a good start, but lets
	 * verify that we really do have a segment with
	 * zero-filesize. */
	dl_iterate_phdr(parse_phdrs, NULL);

	FAIL("Couldn't find zero filesize segment (test misbuilt)");
}

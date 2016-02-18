/*
 * libhugetlbfs - Easy use of Linux hugepages
 * Copyright (C) 2008 Adam Litke, IBM Corporation.
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

#include <elf.h>
#include <link.h>

#include "libhugetlbfs_internal.h"

/*
 * The powerpc 64-bit ELF ABI defines the location and size of the plt as
 * follows (see the ELF ABI and powerpc64 supplement for details):
 *
 * Location:   (data segment p_vaddr) + (data segment p_filesz)
 * Size:       (dynamic symbol table DT_PTRELSZ entry) + 24
 *
 * plt entries have likely been initialized when the libhugetlbfs remapping
 * code runs, we must copy these entries when preparing the data segment.  Tell
 * the arch-independent code how many bytes to copy.
 */
ElfW(Word) plt_extrasz(ElfW(Dyn) *dyntab)
{
	int i;
	ElfW(Word) pltrelsz = 0;

	/* Find the needed information in the dynamic section */
	for (i = 0; dyntab[i].d_tag != DT_NULL; i++)
		if (dyntab[i].d_tag == DT_PLTRELSZ)
			pltrelsz = dyntab[i].d_un.d_val;

	/* pltrelsz indicates the size of all plt entries used to cache
	 * symbol lookups, but does not include the reserved entry at PLT[0].
	 * 24 bytes is the ABI-defined size of a plt entry.
	 */
	if (pltrelsz)
		return pltrelsz + 24;
	else
		return 0;
}

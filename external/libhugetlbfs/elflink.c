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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <link.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include <linux/unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <elf.h>
#include <dlfcn.h>

#include "version.h"
#include "hugetlbfs.h"
#include "libhugetlbfs_internal.h"

#ifdef __LP64__
#define Elf_Ehdr	Elf64_Ehdr
#define Elf_Phdr	Elf64_Phdr
#define Elf_Dyn		Elf64_Dyn
#define Elf_Sym		Elf64_Sym
#define ELF_ST_BIND(x)  ELF64_ST_BIND(x)
#define ELF_ST_TYPE(x)  ELF64_ST_TYPE(x)
#else
#define Elf_Ehdr	Elf32_Ehdr
#define Elf_Phdr	Elf32_Phdr
#define Elf_Dyn		Elf32_Dyn
#define Elf_Sym		Elf32_Sym
#define ELF_ST_BIND(x)  ELF64_ST_BIND(x)
#define ELF_ST_TYPE(x)  ELF64_ST_TYPE(x)
#endif

/*
 * SHARED_TIMEOUT is used by find_or_prepare_shared_file for when it
 * should timeout while waiting for other users to finish preparing
 * the file it wants.  The value is the number of tries before giving
 * up with a 1 second wait between tries
 */
#define SHARED_TIMEOUT 10

/* This function prints an error message to stderr, then aborts.  It
 * is safe to call, even if the executable segments are presently
 * unmapped.
 *
 * Arguments are printf() like, but at present supports only %d and %p
 * with no modifiers
 *
 * FIXME: This works in practice, but I suspect it
 * is not guaranteed safe: the library functions we call could in
 * theory call other functions via the PLT which will blow up. */
static void write_err(const char *start, int len)
{
	direct_syscall(__NR_write, 2 /*stderr*/, start, len);
}
static void sys_abort(void)
{
	pid_t pid = direct_syscall(__NR_getpid);

	direct_syscall(__NR_kill, pid, SIGABRT);
}
static void write_err_base(unsigned long val, int base)
{
	const char digit[] = "0123456789abcdef";
	char str1[sizeof(val)*8];
	char str2[sizeof(val)*8];
	int len = 0;
	int i;

	str1[0] = '0';
	while (val) {
		str1[len++] = digit[val % base];
		val /= base;
	}

	if (len == 0)
		len = 1;

	/* Reverse digits */
	for (i = 0; i < len; i++)
		str2[i] = str1[len-i-1];

	write_err(str2, len);
}

static void unmapped_abort(const char *fmt, ...)
{
	const char *p, *q;
	int done = 0;
	unsigned long val;
	va_list ap;

	/* World's worst printf()... */
	va_start(ap, fmt);
	p = q = fmt;
	while (! done) {
		switch (*p) {
		case '\0':
			write_err(q, p-q);
			done = 1;
			break;

		case '%':
			write_err(q, p-q);
			p++;
			switch (*p) {
			case 'u':
				val = va_arg(ap, unsigned);
				write_err_base(val, 10);
				p++;
				break;
			case 'p':
				val = (unsigned long)va_arg(ap, void *);
				write_err_base(val, 16);
				p++;
				break;
			}
			q = p;
			break;
		default:
			p++;
		}
	}

	va_end(ap);

	sys_abort();
}

/* The directory to use for sharing readonly segments */
static char share_readonly_path[PATH_MAX+1];

#define MAX_HTLB_SEGS	3
#define MAX_SEGS	10

struct seg_info {
	void *vaddr;
	unsigned long filesz, memsz, extrasz;
	int prot;
	int fd;
	int index;
	long page_size;
};

struct seg_layout {
	unsigned long start, end;
	long page_size;
};

static struct seg_info htlb_seg_table[MAX_HTLB_SEGS];
static int htlb_num_segs;
static unsigned long force_remap; /* =0 */
static long hpage_readonly_size, hpage_writable_size;

/**
 * assemble_path - handy wrapper around snprintf() for building paths
 * @dst: buffer of size PATH_MAX+1 to assemble string into
 * @fmt: format string for path
 * @...: printf() style parameters for path
 *
 * assemble_path() builds a path in the target buffer (which must have
 * PATH_MAX+1 available bytes), similar to sprintf().  However, f the
 * assembled path would exceed PATH_MAX characters in length,
 * assemble_path() prints an error and abort()s, so there is no need
 * to check the return value and backout.
 */
static void assemble_path(char *dst, const char *fmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(dst, PATH_MAX+1, fmt, ap);
	va_end(ap);

	if (len < 0) {
		ERROR("vsnprintf() error\n");
		abort();
	}

	if (len > PATH_MAX) {
		ERROR("Overflow assembling path\n");
		abort();
	}
}

static void check_memsz()
{
	int i;
	unsigned long memsz_total = 0, memsz_max = 0;
	if (htlb_num_segs == 0)
		return;
	/*
	 * rough heuristic to see if we'll run out of address
	 * space
	 */
	for (i = 0; i < htlb_num_segs; i++) {
		memsz_total += htlb_seg_table[i].memsz;
		if (htlb_seg_table[i].memsz > memsz_max)
			memsz_max = htlb_seg_table[i].memsz;
	}
	/* avoid overflow checking by using two checks */
	DEBUG("Total memsz = %#0lx, memsz of largest segment = %#0lx\n",
			memsz_total, memsz_max);
}

/**
 * find_or_create_share_path - obtain a directory to store the shared
 * hugetlbfs files
 *
 * Checks environment and filesystem to locate a suitable directory
 * for shared hugetlbfs files, creating a new directory if necessary.
 * The determined path is stored in global variable share_readonly_path.
 *
 * returns:
 *  -1, on error
 *  0, on success
 */
static int find_or_create_share_path(long page_size)
{
	const char *base_path;
	struct stat sb;
	int ret;

	/* If no remaping is planned for the read-only segments we are done */
	if (!page_size)
		return 0;

	if (__hugetlb_opts.share_path) {
		/* Given an explicit path */
		if (hugetlbfs_test_path(__hugetlb_opts.share_path) != 1) {
			WARNING("HUGETLB_SHARE_PATH %s is not on a hugetlbfs"
			      " filesystem\n", __hugetlb_opts.share_path);
			return -1;
		}

		/* Make sure the page size matches */
		if (page_size !=
			hugetlbfs_test_pagesize(__hugetlb_opts.share_path)) {
			WARNING("HUGETLB_SHARE_PATH %s is not valid for a %li "
			      "kB page size\n", __hugetlb_opts.share_path,
				page_size / 1024);
			return -1;
		}
		assemble_path(share_readonly_path, "%s",
				__hugetlb_opts.share_path);
		return 0;
	}

	base_path = hugetlbfs_find_path_for_size(page_size);
	if (!base_path)
		return -1;

	assemble_path(share_readonly_path, "%s/elflink-uid-%d",
			base_path, getuid());

	ret = mkdir(share_readonly_path, 0700);
	if ((ret != 0) && (errno != EEXIST)) {
		WARNING("Error creating share directory %s\n",
			share_readonly_path);
		return -1;
	}

	/* Check the share directory is sane */
	ret = lstat(share_readonly_path, &sb);
	if (ret != 0) {
		WARNING("Couldn't stat() %s: %s\n", share_readonly_path,
			strerror(errno));
		return -1;
	}

	if (! S_ISDIR(sb.st_mode)) {
		WARNING("%s is not a directory\n", share_readonly_path);
		return -1;
	}

	if (sb.st_uid != getuid()) {
		WARNING("%s has wrong owner (uid=%d instead of %d)\n",
		      share_readonly_path, sb.st_uid, getuid());
		return -1;
	}

	if (sb.st_mode & (S_IWGRP | S_IWOTH)) {
		WARNING("%s has bad permissions 0%03o\n",
		      share_readonly_path, sb.st_mode);
		return -1;
	}

	return 0;
}

/*
 * Look for non-zero BSS data inside a range and print out any matches
 */

static void check_bss(unsigned long *start, unsigned long *end)
{
	unsigned long *addr;

	for (addr = start; addr < end; addr++) {
		if (*addr != 0)
			DEBUG("Non-zero BSS data @ %p: %lx\n", addr, *addr);
	}
}

/**
 * get_shared_file_name - create a shared file name from program name,
 * segment number and current word size
 * @htlb_seg_info: pointer to program's segment data
 * @file_path: pointer to a PATH_MAX+1 array to store filename in
 *
 * The file name created is *not* intended to be unique, except when
 * the name, gid or phdr number differ. The goal here is to have a
 * standard means of accessing particular segments of particular
 * executables.
 *
 * returns:
 *   -1, on failure
 *   0, on success
 */
static int get_shared_file_name(struct seg_info *htlb_seg_info, char *file_path)
{
	int ret;
	char binary[PATH_MAX+1];
	char *binary2;

	memset(binary, 0, sizeof(binary));
	ret = readlink("/proc/self/exe", binary, PATH_MAX);
	if (ret < 0) {
		WARNING("shared_file: readlink() on /proc/self/exe "
		      "failed: %s\n", strerror(errno));
		return -1;
	}

	binary2 = basename(binary);
	if (!binary2) {
		WARNING("shared_file: basename() on %s failed: %s\n",
		      binary, strerror(errno));
		return -1;
	}

	assemble_path(file_path, "%s/%s_%zd_%d", share_readonly_path, binary2,
		      sizeof(unsigned long) * 8, htlb_seg_info->index);

	return 0;
}

/* Find the .dynamic program header */
static int find_dynamic(Elf_Dyn **dyntab, const Elf_Phdr *phdr, int phnum)
{
	int i = 1;

	while ((phdr[i].p_type != PT_DYNAMIC) && (i < phnum)) {
		++i;
	}
	if (phdr[i].p_type == PT_DYNAMIC) {
		*dyntab = (Elf_Dyn *)phdr[i].p_vaddr;
		return 0;
	} else {
		DEBUG("No dynamic segment found\n");
		return -1;
	}
}

/* Find the dynamic string and symbol tables */
static int find_tables(Elf_Dyn *dyntab, Elf_Sym **symtab, char **strtab)
{
	int i = 1;
	while ((dyntab[i].d_tag != DT_NULL)) {
		if (dyntab[i].d_tag == DT_SYMTAB)
			*symtab = (Elf_Sym *)dyntab[i].d_un.d_ptr;
		else if (dyntab[i].d_tag == DT_STRTAB)
			*strtab = (char *)dyntab[i].d_un.d_ptr;
		i++;
	}

	if (!*symtab) {
		DEBUG("No symbol table found\n");
		return -1;
	}
	if (!*strtab) {
		DEBUG("No string table found\n");
		return -1;
	}
	return 0;
}

/* Find the number of symbol table entries */
static int find_numsyms(Elf_Sym *symtab, char *strtab)
{
	/*
	 * WARNING - The symbol table size calculation does not follow the ELF
	 *           standard, but rather exploits an assumption we enforce in
	 *           our linker scripts that the string table follows
	 *           immediately after the symbol table. The linker scripts
	 *           must maintain this assumption or this code will break.
	 */
	if ((void *)strtab <= (void *)symtab) {
		DEBUG("Could not calculate dynamic symbol table size\n");
		return -1;
	}
	return ((void *)strtab - (void *)symtab) / sizeof(Elf_Sym);
}

/*
 * To reduce the size of the extra copy window, we can eliminate certain
 * symbols based on information in the dynamic section. The following
 * characteristics apply to symbols which may require copying:
 * - Within the BSS
 * - Global or Weak binding
 * - Object type (variable)
 * - Non-zero size (zero size means the symbol is just a marker with no data)
 */
static inline int keep_symbol(char *strtab, Elf_Sym *s, void *start, void *end)
{
	if ((void *)s->st_value < start)
		return 0;
	if ((void *)s->st_value > end)
		return 0;
	if ((ELF_ST_BIND(s->st_info) != STB_GLOBAL) &&
	    (ELF_ST_BIND(s->st_info) != STB_WEAK))
		return 0;
	if (ELF_ST_TYPE(s->st_info) != STT_OBJECT)
		return 0;
	if (s->st_size == 0)
		return 0;

	if (__hugetlbfs_debug)
		DEBUG("symbol to copy at %p: %s\n", (void *)s->st_value,
						strtab + s->st_name);

	return 1;
}

/* If unspecified by the architecture, no extra copying of the plt is needed */
ElfW(Word) __attribute__ ((weak)) plt_extrasz(ElfW(Dyn) *dyntab)
{
	return 0;
}

/*
 * Subtle:  Since libhugetlbfs depends on glibc, we allow it
 * it to be loaded before us.  As part of its init functions, it
 * initializes stdin, stdout, and stderr in the bss.  We need to
 * include these initialized variables in our copy.
 */

static void get_extracopy(struct seg_info *seg, const Elf_Phdr *phdr, int phnum)
{
	Elf_Dyn *dyntab;        /* dynamic segment table */
	Elf_Sym *symtab = NULL; /* dynamic symbol table */
	Elf_Sym *sym;           /* a symbol */
	char *strtab = NULL;    /* string table for dynamic symbols */
	int ret, numsyms, found_sym = 0;
	void *start, *end, *end_orig;
	void *sym_end;
	void *plt_end;

	end_orig = seg->vaddr + seg->memsz;
	start = seg->vaddr + seg->filesz;
	if (seg->filesz == seg->memsz)
		return;
	if (!__hugetlb_opts.min_copy)
		goto bail2;

	/* Find dynamic program header */
	ret = find_dynamic(&dyntab, phdr, phnum);
	if (ret < 0)
		goto bail;

	/* Find symbol and string tables */
	ret = find_tables(dyntab, &symtab, &strtab);
	if (ret < 0)
		goto bail;

	numsyms = find_numsyms(symtab, strtab);
	if (numsyms < 0)
		goto bail;

	/*
	 * We must ensure any returns done hereafter have sane start and end
	 * values, as the criss-cross apple sauce algorithm is beginning
	 */
	end = start;

	for (sym = symtab; sym < symtab + numsyms; sym++) {
		if (!keep_symbol(strtab, sym, start, end_orig))
			continue;

		/* These are the droids we are looking for */
		found_sym = 1;
		sym_end = (void *)(sym->st_value + sym->st_size);
		if (sym_end > end)
			end = sym_end;
	}

	/*
	 * Some platforms (PowerPC 64bit ELF) place their PLT beyond the filesz
	 * part of the data segment.  When this is the case, we must extend the
	 * copy window to include this data which has been initialized by the
	 * run-time linker.
	 */
	plt_end = start + plt_extrasz(dyntab);
	if (plt_end > end) {
		end = plt_end;
		found_sym = 1;
	}

	if (__hugetlbfs_debug)
		check_bss(end, end_orig);

	if (found_sym) {
		seg->extrasz = end - start;
	}
	/*
	 * else no need to copy anything, so leave seg->extrasz as zero
	 */
	return;

bail:
	DEBUG("Unable to perform minimal copy\n");
bail2:
	seg->extrasz = end_orig - start;
}

#if defined(__powerpc64__) || \
	(defined(__powerpc__) && !defined(PPC_NO_SEGMENTS))
#define SLICE_LOW_TOP		(0x100000000UL)
#define SLICE_LOW_SIZE		(1UL << SLICE_LOW_SHIFT)
#define SLICE_HIGH_SIZE		(1UL << SLICE_HIGH_SHIFT)
#endif

/*
 * Return the address of the start and end of the hugetlb slice
 * containing @addr. A slice is a range of addresses, start inclusive
 * and end exclusive.
 * Note, that since relinking is not supported on ia64, we can leave it
 * out here.
 */
static unsigned long hugetlb_slice_start(unsigned long addr)
{
#if defined(__powerpc64__)
	if (addr < SLICE_LOW_TOP)
		return ALIGN_DOWN(addr, SLICE_LOW_SIZE);
	else if (addr < SLICE_HIGH_SIZE)
		return SLICE_LOW_TOP;
	else
		return ALIGN_DOWN(addr, SLICE_HIGH_SIZE);
#elif defined(__powerpc__) && !defined(PPC_NO_SEGMENTS)
	return ALIGN_DOWN(addr, SLICE_LOW_SIZE);
#else
	return ALIGN_DOWN(addr, gethugepagesize());
#endif
}

static unsigned long hugetlb_slice_end(unsigned long addr)
{
#if defined(__powerpc64__)
	if (addr < SLICE_LOW_TOP)
		return ALIGN_UP(addr, SLICE_LOW_SIZE) - 1;
	else
		return ALIGN_UP(addr, SLICE_HIGH_SIZE) - 1;
#elif defined(__powerpc__) && !defined(PPC_NO_SEGMENTS)
	return ALIGN_UP(addr, SLICE_LOW_SIZE) - 1;
#else
	return ALIGN_UP(addr, gethugepagesize()) - 1;
#endif
}

static unsigned long hugetlb_next_slice_start(unsigned long addr)
{
	return hugetlb_slice_end(addr) + 1;
}

static unsigned long hugetlb_prev_slice_end(unsigned long addr)
{
	return hugetlb_slice_start(addr) - 1;
}

/*
 * Store a copy of the given program header
 */
static int save_phdr(int table_idx, int phnum, const ElfW(Phdr) *phdr)
{
	int prot = 0;

	if (table_idx >= MAX_HTLB_SEGS) {
		WARNING("Executable has too many segments (max %d)\n",
			MAX_HTLB_SEGS);
		htlb_num_segs = 0;
		return -1;
	}

	if (phdr->p_flags & PF_R)
		prot |= PROT_READ;
	if (phdr->p_flags & PF_W)
		prot |= PROT_WRITE;
	if (phdr->p_flags & PF_X)
		prot |= PROT_EXEC;

	htlb_seg_table[table_idx].vaddr = (void *) phdr->p_vaddr;
	htlb_seg_table[table_idx].filesz = phdr->p_filesz;
	htlb_seg_table[table_idx].memsz = phdr->p_memsz;
	htlb_seg_table[table_idx].prot = prot;
	htlb_seg_table[table_idx].index = phnum;

	INFO("Segment %d (phdr %d): %#0lx-%#0lx  (filesz=%#0lx) "
		"(prot = %#0x)\n", table_idx, phnum,
		(unsigned long)  phdr->p_vaddr,
		(unsigned long) phdr->p_vaddr + phdr->p_memsz,
		(unsigned long) phdr->p_filesz, (unsigned int) prot);

	return 0;
}

static int verify_segment_layout(struct seg_layout *segs, int num_segs)
{
	int i;
	long base_size = getpagesize();

	for (i = 1; i < num_segs; i++) {
		unsigned long prev_end = segs[i - 1].end;
		unsigned long start = segs[i].start;

		/*
		 * Do not worry about the boundary between segments that will
		 * not be remapped.
		 */
		if (segs[i - 1].page_size == base_size &&
				segs[i].page_size == base_size)
			continue;

		/* Make sure alignment hasn't caused segments to overlap */
		if (prev_end > start) {
			WARNING("Layout problem with segments %i and %i:\n\t"
				"Segments would overlap\n", i - 1, i);
			return 1;
		}

		/* Make sure page size transitions occur on slice boundaries */
		if ((segs[i - 1].page_size != segs[i].page_size) &&
				hugetlb_slice_end(prev_end) >
				hugetlb_slice_start(start)) {
			WARNING("Layout problem with segments %i and %i:\n\t"
				"Only one page size per slice\n", i - 1, i);
			return 1;
		}
	}
	return 0;
}

static long segment_requested_page_size(const ElfW(Phdr) *phdr)
{
	int writable = phdr->p_flags & PF_W;

	/* Check if a page size was requested by the user */
	if (writable && hpage_writable_size)
		return hpage_writable_size;
	if (!writable && hpage_readonly_size)
		return hpage_readonly_size;

	/* Check if this segment requests remapping by default */
	if (!hpage_readonly_size && !hpage_writable_size &&
			(phdr->p_flags & PF_LINUX_HUGETLB))
		return gethugepagesize();

	/* No remapping selected, return the base page size */
	return getpagesize();
}

static
int parse_elf_normal(struct dl_phdr_info *info, size_t size, void *data)
{
	int i, num_segs;
	unsigned long page_size, seg_psize, start, end;
	struct seg_layout segments[MAX_SEGS];

	page_size = getpagesize();
	num_segs = 0;

	for (i = 0; i < info->dlpi_phnum; i++) {
		if (info->dlpi_phdr[i].p_type != PT_LOAD)
			continue;

		if (i >= MAX_SEGS) {
			WARNING("Maximum number of PT_LOAD segments"
					"exceeded\n");
			return 1;
		}

		seg_psize = segment_requested_page_size(&info->dlpi_phdr[i]);
		if (seg_psize != page_size) {
			if (save_phdr(htlb_num_segs, i, &info->dlpi_phdr[i]))
				return 1;
			get_extracopy(&htlb_seg_table[htlb_num_segs],
					&info->dlpi_phdr[0], info->dlpi_phnum);
			htlb_seg_table[htlb_num_segs].page_size = seg_psize;
			htlb_num_segs++;
		}
		start = ALIGN_DOWN(info->dlpi_phdr[i].p_vaddr, seg_psize);
		end = ALIGN(info->dlpi_phdr[i].p_vaddr +
				info->dlpi_phdr[i].p_memsz, seg_psize);

		segments[num_segs].page_size = seg_psize;
		segments[num_segs].start = start;
		segments[num_segs].end = end;
		num_segs++;
	}
	if (verify_segment_layout(segments, num_segs))
		htlb_num_segs = 0;

	if (__hugetlbfs_debug)
		check_memsz();

	return 1;
}

/*
 * Parse the phdrs of a normal program to attempt partial segment remapping
 */
static
int parse_elf_partial(struct dl_phdr_info *info, size_t size, void *data)
{
	unsigned long vaddr, memsz, gap;
	unsigned long slice_end;
	int i;

	/* This should never actually be called more than once in an
	 * iteration: we assume that dl_iterate_phdrs() always gives
	 * us the main program's phdrs on the first iteration, and
	 * always return 1 to cease iteration at that point. */

	for (i = 0; i < info->dlpi_phnum; i++) {
		if (info->dlpi_phdr[i].p_type != PT_LOAD)
			continue;

		/*
		 * Partial segment remapping only makes sense if the
		 * memory size of the segment is larger than the
		 * granularity at which hugepages can be used. This
		 * mostly affects ppc, where the segment must be larger
		 * than 256M. This guarantees that remapping the binary
		 * in this forced way won't violate any contiguity
		 * constraints.
		 */
		vaddr = hugetlb_next_slice_start(info->dlpi_phdr[i].p_vaddr);
		gap = vaddr - info->dlpi_phdr[i].p_vaddr;
		slice_end = hugetlb_slice_end(vaddr);
		/*
		 * we should stop remapping just before the slice
		 * containing the end of the memsz portion (taking away
		 * the gap of the memsz)
		 */
		memsz = info->dlpi_phdr[i].p_memsz;
		if (memsz < gap) {
			INFO("Segment %d's unaligned memsz is too small: "
					"%#0lx < %#0lx\n",
					i, memsz, gap);
			continue;
		}
		memsz -= gap;
		if (memsz < (slice_end - vaddr)) {
			INFO("Segment %d's aligned memsz is too small: "
					"%#0lx < %#0lx\n",
					i, memsz, slice_end - vaddr);
			continue;
		}
		memsz = hugetlb_prev_slice_end(vaddr + memsz) - vaddr;

		if (save_phdr(htlb_num_segs, i, &info->dlpi_phdr[i]))
			return 1;

		/*
		 * When remapping partial segments, we create a sub-segment
		 * that is based on the original.  For this reason, we must
		 * make some changes to the phdr captured by save_phdr():
		 * 	vaddr is aligned upwards to a slice boundary
		 * 	memsz is aligned downwards to a slice boundary
		 * 	filesz is set to memsz to force all memory to be copied
		 */
		htlb_seg_table[htlb_num_segs].vaddr = (void *)vaddr;
		htlb_seg_table[htlb_num_segs].filesz = memsz;
		htlb_seg_table[htlb_num_segs].memsz = memsz;

		htlb_num_segs++;
	}
	return 1;
}

/*
 * Verify that a range of memory is unoccupied and usable
 */
static void check_range_empty(void *addr, unsigned long len)
{
	void *p;

	p = mmap(addr, len, PROT_READ, MAP_PRIVATE|MAP_ANON, 0, 0);
	if (p != addr) {
		WARNING("Unable to verify address range %p - %p.  Not empty?\n",
				addr, addr + len);
		if (__hugetlbfs_debug)
			dump_proc_pid_maps();
	}
	if (p != MAP_FAILED)
		munmap(p, len);
}

/*
 * Copy a program segment into a huge page. If possible, try to copy the
 * smallest amount of data possible, unless the user disables this
 * optimization via the HUGETLB_ELFMAP environment variable.
 */
static int prepare_segment(struct seg_info *seg)
{
	void *start, *p, *end, *new_end;
	unsigned long size, offset;
	long page_size = getpagesize();
	long hpage_size;
	int mmap_reserve = __hugetlb_opts.no_reserve ? MAP_NORESERVE : 0;

	hpage_size = seg->page_size;

	/*
	 * mmaps must begin at an address aligned to the page size.  If the
	 * vaddr of this segment is not hpage_size aligned, align it downward
	 * and begin the mmap there.  Note the offset so we can copy data to
	 * the correct starting address within the temporary mmap.
	 */
	start = (void *) ALIGN_DOWN((unsigned long)seg->vaddr, hpage_size);
	offset = seg->vaddr - start;

	/*
	 * Calculate the size of the temporary mapping we must create.
	 * This includes the offset (described above) and the filesz and
	 * extrasz portions of the segment (described below).  We must align
	 * this total to the huge page size so it will be valid for mmap.
	 */
	size = ALIGN(offset + seg->filesz + seg->extrasz, hpage_size);

	/*
	 * If the segment's start or end addresses have been adjusted to align
	 * them to the hpage_size, check to make sure nothing is mapped in the
	 * padding before and after the segment.
	 */
	end = (void *) ALIGN((unsigned long)seg->vaddr + seg->memsz, page_size);
	new_end = (void *) ALIGN((unsigned long)end, hpage_size);
	if (ALIGN_DOWN(offset, page_size))
		check_range_empty(start, ALIGN_DOWN(offset, page_size));
	if (end != new_end)
		check_range_empty(end, new_end - end);

	/* Create the temporary huge page mmap */
	p = mmap(NULL, size, PROT_READ|PROT_WRITE,
				MAP_SHARED|mmap_reserve, seg->fd, 0);
	if (p == MAP_FAILED) {
		WARNING("Couldn't map hugepage segment to copy data: %s\n",
			strerror(errno));
		return -1;
	}

	/*
	 * Minimizing the amount of data copied will maximize performance.
	 * By definition, the filesz portion of the segment contains
	 * initialized data and must be copied.  If part of the memsz portion
	 * is known to be initialized already, extrasz will be non-zero and
	 * that many addtional bytes will be copied from the beginning of the
	 * memsz region.  The rest of the memsz is understood to be zeroes and
	 * need not be copied.
	 */
	INFO("Mapped hugeseg at %p. Copying %#0lx bytes and %#0lx extra bytes"
		" from %p...", p, seg->filesz, seg->extrasz, seg->vaddr);
	memcpy(p + offset, seg->vaddr, seg->filesz + seg->extrasz);
	INFO_CONT("done\n");

	munmap(p, size);

	return 0;
}

/*
 * [PPC] Prior to 2.6.22 (which added slices), our temporary hugepage
 * mappings are placed in the segment before the stack. This 'taints' that
 * segment for be hugepage-only for the lifetime of the process, resulting
 * in a maximum stack size of 256MB. If we instead create our hugepage
 * mappings in a child process, we can avoid this problem.
 *
 * This does not adversely affect non-PPC platforms so do it everywhere.
 */
static int fork_and_prepare_segment(struct seg_info *htlb_seg_info)
{
	int pid, ret, status;

	if ((pid = fork()) < 0) {
		WARNING("fork failed");
		return -1;
	}
	if (pid == 0) {
		ret = prepare_segment(htlb_seg_info);
		if (ret < 0) {
			WARNING("Failed to prepare segment\n");
			exit(1);
		}
		else
			exit(0);
	}
	ret = waitpid(pid, &status, 0);
	if (ret == -1) {
		WARNING("waitpid failed");
		return -1;
	}

	if (WEXITSTATUS(status) != 0)
		return -1;

	INFO("Prepare succeeded\n");
	return 0;
}

/**
 * find_or_prepare_shared_file - get one shareable file
 * @htlb_seg_info: pointer to program's segment data
 *
 * This function either locates a hugetlbfs file already containing
 * data for a given program segment, or creates one if it doesn't
 * already exist.
 *
 * We use the following algorithm to ensure that when processes race
 * to instantiate the hugepage file, we will never obtain an
 * incompletely prepared file or have multiple processes prepar
 * separate copies of the file.
 *	- first open 'filename.tmp' with O_EXCL (this acts as a lockfile)
 *	- second open 'filename' with O_RDONLY (even if the first open
 *	  succeeded).
 * Then:
 * 	- If both opens succeed, close the O_EXCL open, unlink
 * filename.tmp and use the O_RDONLY fd.  (Somebody else has prepared
 * the file already)
 * 	- If only the O_RDONLY open suceeds, and the O_EXCL open
 * fails with EEXIST, just used the O_RDONLY fd. (Somebody else has
 * prepared the file already, but we raced with their rename()).
 * 	- If only the O_EXCL open suceeds, and the O_RDONLY fails with
 * ENOENT, prepare the the O_EXCL open, then rename() filename.tmp to
 * filename. (We're the first in, we have to prepare the file).
 * 	- If both opens fail, with EEXIST and ENOENT, respectively,
 * wait for a little while, then try again from the beginning
 * (Somebody else is preparing the file, but hasn't finished yet)
 *
 * returns:
 *   -1, on failure
 *   0, on success
 */
static int find_or_prepare_shared_file(struct seg_info *htlb_seg_info)
{
	int fdx = -1, fds;
	int errnox, errnos;
	int ret;
	int i;
	char final_path[PATH_MAX+1];
	char tmp_path[PATH_MAX+1];

	ret = get_shared_file_name(htlb_seg_info, final_path);
	if (ret < 0)
		return -1;
	assemble_path(tmp_path, "%s.tmp", final_path);

	for (i = 0; i < SHARED_TIMEOUT; i++) {
		/* NB: mode is modified by umask */
		fdx = open(tmp_path, O_CREAT | O_EXCL | O_RDWR, 0666);
		errnox = errno;
		fds = open(final_path, O_RDONLY);
		errnos = errno;

		if (fds >= 0) {
			/* Got an already-prepared file -> use it */
			if (fdx > 0) {
				/* Also got an exclusive file -> clean up */
				ret = unlink(tmp_path);
				if (ret != 0)
					WARNING("shared_file: unable to clean "
					      "up unneeded file %s: %s\n",
					      tmp_path, strerror(errno));
				close(fdx);
			} else if (errnox != EEXIST) {
				WARNING("shared_file: Unexpected failure on exclusive"
					" open of %s: %s\n", tmp_path,
					strerror(errnox));
			}
			htlb_seg_info->fd = fds;
			return 0;
		}

		if (fdx >= 0) {
			/* It's our job to prepare */
			if (errnos != ENOENT)
				WARNING("shared_file: Unexpected failure on"
					" shared open of %s: %s\n", final_path,
					strerror(errnos));

			htlb_seg_info->fd = fdx;

			INFO("Got unpopulated shared fd -- Preparing\n");
			ret = fork_and_prepare_segment(htlb_seg_info);
			if (ret < 0)
				goto fail;

			INFO("Prepare succeeded\n");
			/* move to permanent location */
			ret = rename(tmp_path, final_path);
			if (ret != 0) {
				WARNING("shared_file: unable to rename %s"
				      " to %s: %s\n", tmp_path, final_path,
				      strerror(errno));
				goto fail;
			}

			return 0;
		}

		/* Both opens failed, somebody else is still preparing */
		/* Wait and try again */
		sleep(1);
	}

 fail:
	if (fdx > 0) {
		ret = unlink(tmp_path);
		if (ret != 0)
			WARNING("shared_file: Unable to clean up temp file %s "
			      "on failure: %s\n", tmp_path, strerror(errno));
		close(fdx);
	}

	return -1;
}

/**
 * obtain_prepared_file - multiplex callers depending on if
 * sharing or not
 * @htlb_seg_info: pointer to program's segment data
 *
 * returns:
 *  -1, on error
 *  0, on success
 */
static int obtain_prepared_file(struct seg_info *htlb_seg_info)
{
	int fd = -1;
	int ret;
	long hpage_size = htlb_seg_info->page_size;

	/* Share only read-only segments */
	if (__hugetlb_opts.sharing && !(htlb_seg_info->prot & PROT_WRITE)) {
		/* first, try to share */
		ret = find_or_prepare_shared_file(htlb_seg_info);
		if (ret == 0)
			return 0;
		/* but, fall through to unlinked files, if sharing fails */
		WARNING("Falling back to unlinked files\n");
	}
	fd = hugetlbfs_unlinked_fd_for_size(hpage_size);
	if (fd < 0)
		return -1;
	htlb_seg_info->fd = fd;

	return fork_and_prepare_segment(htlb_seg_info);
}

static void remap_segments(struct seg_info *seg, int num)
{
	int i;
	void *p;
	unsigned long start, offset, mapsize;
	long page_size = getpagesize();
	long hpage_size;
	int mmap_flags;

	/*
	 * XXX: The bogus call to mmap below forces ld.so to resolve the
	 * mmap symbol before we unmap the plt in the data segment
	 * below.  This might only be needed in the case where sharing
	 * is enabled and the hugetlbfs files have already been prepared
	 * by another process.
	 */
	 p = mmap(0, 0, 0, 0, 0, 0);

	/* This is the hairy bit, between unmap and remap we enter a
	 * black hole.  We can't call anything which uses static data
	 * (ie. essentially any library function...)
	 */
	for (i = 0; i < num; i++) {
		start = ALIGN_DOWN((unsigned long)seg[i].vaddr, page_size);
		offset = (unsigned long)(seg[i].vaddr - start);
		mapsize = ALIGN(offset + seg[i].memsz, page_size);
		munmap((void *) start, mapsize);
	}

	/* Step 4.  Rebuild the address space with hugetlb mappings */
	/* NB: we can't do the remap as hugepages within the main loop
	 * because of PowerPC: we may need to unmap all the normal
	 * segments before the MMU segment is ok for hugepages */
	for (i = 0; i < num; i++) {
		hpage_size = seg[i].page_size;
		start = ALIGN_DOWN((unsigned long)seg[i].vaddr, hpage_size);
		offset = (unsigned long)(seg[i].vaddr - start);
		mapsize = ALIGN(offset + seg[i].memsz, hpage_size);
		mmap_flags = MAP_PRIVATE|MAP_FIXED;

		/* If requested, make no reservations */
		if (__hugetlb_opts.no_reserve)
			mmap_flags |= MAP_NORESERVE;

		/*
		 * If this is a read-only mapping whose contents are
		 * entirely contained within the file, then use MAP_NORESERVE.
		 * The assumption is that the pages already exist in the
		 * page cache for the hugetlbfs file since it was prepared
		 * earlier and that mprotect() will not be called which would
		 * require a COW
		 */
		if (!(seg[i].prot & PROT_WRITE) &&
				seg[i].filesz == seg[i].memsz)
			mmap_flags |= MAP_NORESERVE;

		p = mmap((void *) start, mapsize, seg[i].prot,
			 mmap_flags, seg[i].fd, 0);
		if (p == MAP_FAILED)
			unmapped_abort("Failed to map hugepage segment %u: "
					"%p-%p (errno=%u)\n", i, start,
					start + mapsize, errno);
		if (p != (void *) start)
			unmapped_abort("Mapped hugepage segment %u (%p-%p) at "
				       "wrong address %p\n", i, seg[i].vaddr,
				       seg[i].vaddr+mapsize, p);
	}
	/* The segments are all back at this point.
	 * and it should be safe to reference static data
	 */
}

static int set_hpage_sizes(const char *env)
{
	char *pos;
	long size;
	char *key;
	char keys[5] = { "R\0" "W\0" "\0" };

	/* For each key in R,W */
	for (key = keys; *key != '\0'; key += 2) {
		pos = strcasestr(env, key);
		if (!pos)
			continue;

		if (*(++pos) == '=') {
			size = parse_page_size(pos + 1);
			if (size == -1)
				return size;
		} else
			size = gethugepagesize();

		if (size <= 0) {
			if (errno == ENOSYS)
				WARNING("Hugepages unavailable\n");
			else if (errno == EOVERFLOW)
				WARNING("Hugepage size too large\n");
			else
				WARNING("Hugepage size (%s)\n",
						strerror(errno));
			size = 0;
		} else if (!hugetlbfs_find_path_for_size(size)) {
			WARNING("Hugepage size %li unavailable", size);
			size = 0;
		}

		if (*key == 'R')
			hpage_readonly_size = size;
		else
			hpage_writable_size = size;
	}
	return 0;
}

static int check_env(void)
{
	extern Elf_Ehdr __executable_start __attribute__((weak));

	if (__hugetlb_opts.elfmap &&
		(strcasecmp(__hugetlb_opts.elfmap, "no") == 0)) {
		INFO("HUGETLB_ELFMAP=%s, not attempting to remap program "
		      "segments\n", __hugetlb_opts.elfmap);
		return -1;
	}
	if (__hugetlb_opts.elfmap && set_hpage_sizes(__hugetlb_opts.elfmap)) {
		WARNING("Cannot set elfmap page sizes: %s", strerror(errno));
		return -1;
	}

	if (__hugetlb_opts.ld_preload &&
		strstr(__hugetlb_opts.ld_preload, "libhugetlbfs")) {
		if (__hugetlb_opts.force_elfmap) {
			force_remap = 1;
			INFO("HUGETLB_FORCE_ELFMAP=yes, "
					"enabling partial segment "
					"remapping for non-relinked "
					"binaries\n");
			INFO("Disabling filesz copy optimization\n");
			__hugetlb_opts.min_copy = false;
		} else {
			if (&__executable_start) {
				WARNING("LD_PRELOAD is incompatible with "
					"segment remapping\n");
				WARNING("Segment remapping has been "
					"DISABLED\n");
				return -1;
			}
		}
	}

	if (__hugetlb_opts.sharing == 2) {
		WARNING("HUGETLB_SHARE=%d, however sharing of writable\n"
			"segments has been deprecated and is now disabled\n",
			__hugetlb_opts.sharing);
		__hugetlb_opts.sharing = 0;
	} else {
		INFO("HUGETLB_SHARE=%d, sharing ", __hugetlb_opts.sharing);
		if (__hugetlb_opts.sharing == 1) {
			INFO_CONT("enabled for only read-only segments\n");
		} else {
			INFO_CONT("disabled\n");
			__hugetlb_opts.sharing = 0;
		}
	}

	INFO("HUGETLB_NO_RESERVE=%s, reservations %s\n",
			__hugetlb_opts.no_reserve ? "yes" : "no",
			__hugetlb_opts.no_reserve ? "disabled" : "enabled");

	return 0;
}

/*
 * Parse an ELF header and record segment information for any segments
 * which contain hugetlb information.
 */
static int parse_elf()
{
	if (force_remap)
		dl_iterate_phdr(parse_elf_partial, NULL);
	else
		dl_iterate_phdr(parse_elf_normal, NULL);

	if (htlb_num_segs == 0) {
		INFO("No segments were appropriate for remapping\n");
		return -1;
	}

	return 0;
}

void hugetlbfs_setup_elflink(void)
{
	int i, ret;

	if (check_env())
		return;

	if (parse_elf())
		return;

	INFO("libhugetlbfs version: %s\n", VERSION);

	/* Do we need to find a share directory */
	if (__hugetlb_opts.sharing) {
		/*
		 * If HUGETLB_ELFMAP is undefined but a shareable segment has
		 * PF_LINUX_HUGETLB set, segment remapping will occur using the
		 * default huge page size.
		 */
		long page_size = hpage_readonly_size ?
			hpage_readonly_size : gethugepagesize();

		ret = find_or_create_share_path(page_size);
		if (ret != 0) {
			WARNING("Segment remapping is disabled");
			return;
		}
	}

	/* Step 1.  Obtain hugepage files with our program data */
	for (i = 0; i < htlb_num_segs; i++) {
		ret = obtain_prepared_file(&htlb_seg_table[i]);
		if (ret < 0) {
			WARNING("Failed to setup hugetlbfs file for segment "
					"%d\n", i);

			/* Close files we have already prepared */
			for (i--; i >= 0; i--)
				close(htlb_seg_table[i].fd);

			return;
		}
	}

	/* Step 3.  Unmap the old segments, map in the new ones */
	remap_segments(htlb_seg_table, htlb_num_segs);
}

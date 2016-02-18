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
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "libhugetlbfs_internal.h"
#include "hugetlbfs.h"
#include <sys/syscall.h>

#if defined(SYS_shmget) || defined(SYS_ipc)
#define HAVE_SHMGET_SYSCALL
#endif

#ifdef HAVE_SHMGET_SYSCALL
/*
 * The calls to dlsym() and dlerror() in the shmget() wrapper below force
 * a dependency on libdl.so.  This does not work for static executables
 * as the glibc dynamic library implementation does not automatically
 * have static dl* function stubs linked into static executables.
 *
 * Work around this problem by adding a weak attribute to the declarations
 * of dlsym() and dlerror().  (The declaration is otherwise the same as in
 * <dlfcn.h>).  This allows a static executable to be linked without -ldl.
 * If &dlsym is NULL then this is a static executable and a call to the
 * system shmget() may be performed without worry as there is no dynamic
 * call chain.
 */
extern void *dlsym (void *__restrict __handle, __const char *__restrict __name)
		__attribute__((weak)) __THROW __nonnull ((2));
extern char *dlerror (void) __attribute__((weak)) __THROW;


/* call syscall shmget through the generic syscall mechanism */
static int syscall_shmget(key_t key, size_t size, int shmflg)
{
#ifdef SYS_shmget
	return syscall(SYS_shmget, key, size, shmflg);
#else
	/*
	 * Some platforms do not have have a direct shmget syscall.  Instead,
	 * all SysV IPC calls are funneled through the ipc() system call.
	 *
	 * ipc() is expected to only be used by libc implementors, so using
	 * it has not been smoothed out.  There is no function declaration.
	 * The needed define for SHMGET is in linux/ipc.h, but that file
	 * also includes a conflicting definition of ipc_perm.  So,
	 * just define the needed items here.
	 *
	 * When compiling -m32 on x86_64, the ipc glibc wrapper does not
	 * exist.  Instead, just use SYS_ipc.
	 *
	 * The ipc system call below does not set the IPC_64 version flag
	 * with SHMGET because that would have required more private defines
	 * and the version number is not used for the SHMGET call.
	 */
	#define SHMGET 23

	return syscall(SYS_ipc, SHMGET, key, size, shmflg, (void *)NULL, 0L);
#endif
}

#endif /* HAVE_SHMGET_SYSCALL */

int shmget(key_t key, size_t size, int shmflg)
{
	static int (*real_shmget)(key_t key, size_t size, int shmflg) = NULL;
	char *error;
	int retval;
	size_t aligned_size = size;

	DEBUG("hugetlb_shmem: entering overridden shmget() call\n");

	/* Get a handle to the "real" shmget system call */
	if (!real_shmget) {
#ifdef HAVE_SHMGET_SYSCALL
		if (&dlsym == NULL) {
			/* in a static executable, call shmget directly */
			real_shmget = syscall_shmget;
		} else
#endif /* HAVE_SHMGET_SYSCALL */
		{
			real_shmget = dlsym(RTLD_NEXT, "shmget");
			if ((error = dlerror()) != NULL) {
				ERROR("%s", error);
				return -1;
			}
		}
	}

	/* Align the size and set SHM_HUGETLB on request */
	if (__hugetlb_opts.shm_enabled) {
		/*
		 * Use /proc/meminfo because shm always uses the system
		 * default huge page size.
		 */
		long hpage_size = kernel_default_hugepage_size();
		aligned_size = ALIGN(size, hpage_size);
		if (size != aligned_size) {
			DEBUG("hugetlb_shmem: size growth align %zd -> %zd\n",
				size, aligned_size);
		}

		INFO("hugetlb_shmem: Adding SHM_HUGETLB flag\n");
		shmflg |= SHM_HUGETLB;
	} else {
		DEBUG("hugetlb_shmem: shmget override not requested\n");
	}

	/* Call the "real" shmget. If hugepages fail, use small pages */
	retval = real_shmget(key, aligned_size, shmflg);
	if (retval == -1 && __hugetlb_opts.shm_enabled) {
		WARNING("While overriding shmget(%zd) to add SHM_HUGETLB: %s\n",
			aligned_size, strerror(errno));
		shmflg &= ~SHM_HUGETLB;
		retval = real_shmget(key, size, shmflg);
		WARNING("Using small pages for shmget despite HUGETLB_SHM\n");
	}

	return retval;
}

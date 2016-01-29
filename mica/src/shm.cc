// Copyright 2014 Carnegie Mellon University
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "shm.h"

#include "util.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifdef USE_DPDK
#include <rte_lcore.h>
#include <rte_debug.h>
#endif

MEHCACHED_BEGIN

struct mehcached_shm_page
{
	char path[PATH_MAX];
	void *addr;
	void *paddr;
	size_t numa_node;
	size_t in_use;
};

struct mehcached_shm_entry
{
	size_t refcount;	// reference by mapping
	size_t to_remove;	// remove entry when refcount == 0
	size_t length;
	size_t num_pages;
	size_t *pages;
};

struct mehcached_shm_mapping
{
	size_t entry_id;
	void *addr;
	size_t length;
	size_t page_offset;
	size_t num_pages;
};

#define MEHCACHED_SHM_MAX_PAGES (65536)
#define MEHCACHED_SHM_MAX_ENTRIES (8192)
#define MEHCACHED_SHM_MAX_MAPPINGS (16384)

static size_t mehcached_shm_page_size;
static uint64_t mehcached_shm_state_lock;
static struct mehcached_shm_page mehcached_shm_pages[MEHCACHED_SHM_MAX_PAGES];
static struct mehcached_shm_entry mehcached_shm_entries[MEHCACHED_SHM_MAX_ENTRIES];
static struct mehcached_shm_mapping mehcached_shm_mappings[MEHCACHED_SHM_MAX_MAPPINGS];
static size_t mehcached_shm_used_memory;

static const char *mehcached_shm_path_prefix = "/mnt/huge/mehcached_shm_";

size_t
mehcached_shm_adjust_size(size_t size)
{
    return (uint64_t)MEHCACHED_ROUNDUP2M(size);
}

static
void
mehcached_clean_files()
{
	char cmd[PATH_MAX];
	snprintf(cmd, PATH_MAX, "rm %s* > /dev/null 2>&1", mehcached_shm_path_prefix);
	int ret = system(cmd);
	(void)ret;
}

static
void
mehcached_shm_path(size_t page_id, char out_path[PATH_MAX])
{
	snprintf(out_path, PATH_MAX, "%s%zu", mehcached_shm_path_prefix, page_id);
}

static
void
mehcached_shm_lock()
{
	while (1)
	{
		if (__sync_bool_compare_and_swap((volatile uint64_t *)&mehcached_shm_state_lock, 0UL, 1UL))
			break;
	}
}

static
void
mehcached_shm_unlock()
{
	memory_barrier();
	*(volatile uint64_t *)&mehcached_shm_state_lock = 0UL;
}

void
mehcached_shm_dump_page_info()
{
	mehcached_shm_lock();
	size_t page_id;
	for (page_id = 0; page_id < MEHCACHED_SHM_MAX_PAGES; page_id++)
	{
		if (mehcached_shm_pages[page_id].addr == NULL)
			continue;

		printf("page %zu: addr=%p numa_node=%zu in_use=%zu\n", page_id, mehcached_shm_pages[page_id].addr, mehcached_shm_pages[page_id].numa_node, mehcached_shm_pages[page_id].in_use);
	}
	mehcached_shm_unlock();
}

static
int
mehcached_shm_compare_paddr(const void *a, const void *b)
{
	const struct mehcached_shm_page *pa = (const struct mehcached_shm_page *)a;
	const struct mehcached_shm_page *pb = (const struct mehcached_shm_page *)b;
	if (pa->paddr < pb->paddr)
		return -1;
	else
		return 1;
}

static
int
mehcached_shm_compare_vaddr(const void *a, const void *b)
{
	const struct mehcached_shm_page *pa = (const struct mehcached_shm_page *)a;
	const struct mehcached_shm_page *pb = (const struct mehcached_shm_page *)b;
	if (pa->addr < pb->addr)
		return -1;
	else
		return 1;
}

void
mehcached_shm_init(size_t page_size, size_t num_numa_nodes, size_t num_pages_to_try, size_t num_pages_to_reserve)
{
	assert(mehcached_next_power_of_two(page_size) == page_size);
	assert(num_numa_nodes >= 1);
	assert(num_pages_to_try <= MEHCACHED_SHM_MAX_PAGES);
	assert(num_pages_to_reserve <= num_pages_to_try);

	size_t page_id;
	size_t numa_node;

	printf("cleaning up existing files\n");
	mehcached_clean_files();

	mehcached_shm_state_lock = 0;
	mehcached_shm_page_size = page_size;
	memset(mehcached_shm_pages, 0, sizeof(mehcached_shm_pages));
	memset(mehcached_shm_entries, 0, sizeof(mehcached_shm_entries));
	memset(mehcached_shm_mappings, 0, sizeof(mehcached_shm_mappings));
	mehcached_shm_used_memory = 0;

	// initialize pages
	printf("initializing pages\n");
	size_t num_allocated_pages;
	for (page_id = 0; page_id < num_pages_to_try; page_id++)
	{
		char path[PATH_MAX];
		mehcached_shm_path(page_id, path);

		int fd = open(path, O_CREAT | O_RDWR, 0755);
		if (fd == -1)
		{
			perror("");
			assert(false);
		}

		void *p = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		close(fd);

		if (p == (void *)-1)
			break;

		// this is required to cause a page fault and invoke actual memory allocation
		*(size_t *)p = 0;

		strncpy(mehcached_shm_pages[page_id].path, path, PATH_MAX);
		mehcached_shm_pages[page_id].addr = p;
		//printf("initial allocation of %zu on %p\n", page_size, p);
	}
	num_allocated_pages = page_id;

	printf("initial allocation of %zu pages\n", num_allocated_pages);

    // sort by virtual address
	printf("sorting by virtual address\n");
    qsort(mehcached_shm_pages, num_allocated_pages, sizeof(struct mehcached_shm_page), mehcached_shm_compare_vaddr);

	// detect numa socket
	printf("detecting NUMA mapping\n");
	FILE *f = fopen("/proc/self/numa_maps", "r");
	if (f == NULL)
	{
		perror("");
		assert(false);
	}

	page_id = 0;
	char buf[BUFSIZ];
	while (true)
	{
		if (fgets(buf, sizeof(buf), f) == NULL)
			break;

		size_t addr = strtoull(buf, NULL, 16);
		// for (page_id = 0; page_id < num_allocated_pages; page_id++)
		if (page_id < num_allocated_pages)
		{
			if (mehcached_shm_pages[page_id].addr == (void *)addr)
			{
				char *p = strstr(buf, " N");
				if (p == NULL)
					assert(false);

				size_t numa_node;
				size_t page_count;
				if (sscanf(p, " N%zu=%zu", &numa_node, &page_count) != 2)
					assert(false);
				if (page_count != 1)
					printf("warning: page count for %p is expected to be 1\n", (void *)addr);

				mehcached_shm_pages[page_id].numa_node = numa_node;
				//printf("%p is on numa node %zu\n", p, numa_node);
				page_id++;
			}
		}
	}
	fclose(f);
	if (page_id != num_allocated_pages)
	{
		printf("error: unable to get NUMA mapping information for all pages (/proc/self/numa_maps may be not sorted by virtual address)\n");
		assert(false);
	}

    // get physical address (pagemap.txt)
	printf("detecting physical address of pages\n");
    size_t normal_page_size = (size_t)getpagesize();
	int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd == -1)
	{
		perror("");
		assert(false);
	}

	for (page_id = 0; page_id < num_allocated_pages; page_id++)
	{
        size_t pfn = (size_t)mehcached_shm_pages[page_id].addr / normal_page_size;
        off_t offset = (off_t)(sizeof(uint64_t) * pfn);

		if (lseek(fd, offset, SEEK_SET) != offset)
        {
            perror("");
            close(fd);
            assert(false);
		}

        uint64_t entry;
		if (read(fd, &entry, sizeof(uint64_t)) == -1)
        {
            perror("");
            close(fd);
            assert(false);
		}

        mehcached_shm_pages[page_id].paddr = (void *)((entry & 0x7fffffffffffffULL) * normal_page_size);
        //printf("virtual addr %p = physical addr %p\n", mehcached_shm_pages[page_id].addr, mehcached_shm_pages[page_id].paddr);
    }

    // sort by physical address
	printf("sorting by physical address\n");
    // for (page_id = 0; page_id < num_allocated_pages - 1; page_id++)
    // {
    //     size_t page_id2;
    //     for (page_id2 = page_id + 1; page_id2 < num_allocated_pages; page_id2++)
    //     {
    //         if (mehcached_shm_pages[page_id].paddr > mehcached_shm_pages[page_id2].paddr)
    //         {
    //             struct mehcached_shm_page t;
    //             memcpy(&t, &mehcached_shm_pages[page_id], sizeof(struct mehcached_shm_page));
    //             memcpy(&mehcached_shm_pages[page_id], &mehcached_shm_pages[page_id2], sizeof(struct mehcached_shm_page));
    //             memcpy(&mehcached_shm_pages[page_id2], &t, sizeof(struct mehcached_shm_page));
    //         }
    //     }
    // }
    qsort(mehcached_shm_pages, num_allocated_pages, sizeof(struct mehcached_shm_page), mehcached_shm_compare_paddr);

	// throw away surplus pages on each numa node
	printf("releasing unnecessary pages\n");
	size_t num_pages_per_numa_node = num_pages_to_reserve / num_numa_nodes;
	size_t num_reserved_pages[num_numa_nodes];
	size_t num_freed_pages[num_numa_nodes];
	memset(num_reserved_pages, 0, sizeof(num_reserved_pages));
	memset(num_freed_pages, 0, sizeof(num_freed_pages));

	for (page_id = 0; page_id < num_allocated_pages; page_id++)
	{
		size_t numa_node = mehcached_shm_pages[page_id].numa_node;
        void *addr = mehcached_shm_pages[page_id].addr;
		if (num_reserved_pages[numa_node] < num_pages_per_numa_node)
        {
            //printf("reserving page (addr=%p, paddr=%p, numa_node=%zu)\n", addr, mehcached_shm_pages[page_id].paddr, numa_node);
			num_reserved_pages[numa_node]++;
            //memset(addr, 0, mehcached_shm_page_size);
        }
        else
		{
			//printf("deallocating surplus page %p on numa node %zu\n", addr, numa_node);

			munmap(addr, mehcached_shm_page_size);
			unlink(mehcached_shm_pages[page_id].path);
			memset(&mehcached_shm_pages[page_id], 0, sizeof(mehcached_shm_pages[page_id]));

			num_freed_pages[numa_node]++;
		}
	}

	for (numa_node = 0; numa_node < num_numa_nodes; numa_node++)
		printf("freed %zu pages on numa node %zu\n", num_freed_pages[numa_node], numa_node);

	// check if we have enough pages on each numa node
	for (numa_node = 0; numa_node < num_numa_nodes; numa_node++)
	{
		if (num_reserved_pages[numa_node] != num_pages_per_numa_node)
			printf("warning: could reserve only %zu pages (< %zu) on numa node %zu\n", num_reserved_pages[numa_node], num_pages_per_numa_node, numa_node);
		printf("reserved %zu pages on numa node %zu\n", num_pages_per_numa_node, numa_node);
	}

	//mehcached_shm_dump_page_info();
}

void *
mehcached_shm_find_free_address(size_t size)
{
	size_t alignment = mehcached_shm_page_size;

	if (alignment == 0)
		alignment = 1;

	if (mehcached_next_power_of_two(alignment) != alignment)
	{
		printf("invalid alignment\n");
		return NULL;
	}

	int fd = open("/dev/zero", O_RDONLY);
	if (fd == -1)
	{
		perror("");
		assert(false);
		return NULL;
	}

	void *p = mmap(NULL, size + alignment, PROT_READ, MAP_PRIVATE, fd, 0);

	close(fd);

	if (p == (void *)-1)
	{
		perror("");
		return NULL;
	}

	munmap(p, size);

	p = (void *)(((size_t)p + (alignment - 1)) & ~(alignment - 1));
	return p;
}

size_t
mehcached_shm_alloc(size_t length, size_t numa_node)
{
	if (numa_node == (size_t)-1)
	{
#ifndef USE_DPDK
		numa_node = 0;
#else
		// using rte_socket_id() is unreliable on fawnserver (physical id of processor 0 is 1 (0 expected))

		unsigned lcore_id = rte_lcore_id();
		for (numa_node = 0; numa_node < 16; numa_node++)
		{
			char path[PATH_MAX];
			snprintf(path, PATH_MAX, "/sys/devices/system/cpu/cpu%u/node%zu", lcore_id, numa_node);
			struct stat s;
			if (lstat(path, &s) == 00)
				break;
		}
		if (numa_node == 16)
		{
			printf("failed to detect numa node for current cpu\n");
			return (size_t)-1;
		}
#endif
	}
	mehcached_shm_lock();

	size_t entry_id;
	for (entry_id = 0; entry_id < MEHCACHED_SHM_MAX_ENTRIES; entry_id++)
	{
		if (mehcached_shm_entries[entry_id].pages == NULL)
			break;
	}

	if (entry_id == MEHCACHED_SHM_MAX_ENTRIES)
	{
		printf("too many entries\n");
		mehcached_shm_unlock();
		return (size_t)-1;
	}

	size_t num_pages = (length + (mehcached_shm_page_size - 1)) / mehcached_shm_page_size;
	mehcached_shm_entries[entry_id].length = length;
	mehcached_shm_entries[entry_id].num_pages = num_pages;
	mehcached_shm_entries[entry_id].pages = (size_t *)malloc(sizeof(size_t) * num_pages);
	size_t num_allocated_pages = 0;

	size_t page_id;
	for (page_id = 0; page_id < MEHCACHED_SHM_MAX_PAGES; page_id++)
	{
		if (num_allocated_pages == num_pages)
			break;

		if (mehcached_shm_pages[page_id].addr == NULL)
			continue;
		if (mehcached_shm_pages[page_id].in_use || mehcached_shm_pages[page_id].numa_node != numa_node)
			continue;

		mehcached_shm_entries[entry_id].pages[num_allocated_pages] = page_id;
		num_allocated_pages++;
	}
	if (num_pages != num_allocated_pages)
	{
		printf("insufficient memory on numa node %zu to allocate %zu bytes\n", numa_node, length);
		free(mehcached_shm_entries[entry_id].pages);
		memset(&mehcached_shm_entries[entry_id], 0, sizeof(mehcached_shm_entries[entry_id]));
		mehcached_shm_unlock();
		return (size_t)-1;
	}

	size_t page_index;
	for (page_index = 0; page_index < num_pages; page_index++)
		mehcached_shm_pages[mehcached_shm_entries[entry_id].pages[page_index]].in_use = 1;

	mehcached_shm_unlock();

#ifndef NDEBUG
	printf("allocated shm entry %zu (length=%zu, num_pages=%zu) on numa node %zu\n", entry_id, length, num_pages, numa_node);
#endif

	//mehcached_shm_dump_page_info();

	return entry_id;
}

static
void
mehcached_shm_check_remove(size_t entry_id)
{
	// lock assumed

	// remove entry if no one uses and scheduled to be removed
	if (mehcached_shm_entries[entry_id].refcount == 0 && mehcached_shm_entries[entry_id].to_remove != 0)
	{
		size_t page_index;
		for (page_index = 0; page_index < mehcached_shm_entries[entry_id].num_pages; page_index++)
			mehcached_shm_pages[mehcached_shm_entries[entry_id].pages[page_index]].in_use = 0;
		free(mehcached_shm_entries[entry_id].pages);
		memset(&mehcached_shm_entries[entry_id], 0, sizeof(mehcached_shm_entries[entry_id]));

		mehcached_shm_used_memory -= mehcached_shm_entries[entry_id].num_pages * mehcached_shm_page_size;
#ifndef NDEBUG
		printf("deallocated shm entry %zu\n", entry_id);
#endif
	}
}

bool
mehcached_shm_schedule_remove(size_t entry_id)
{
	mehcached_shm_lock();

	if (mehcached_shm_entries[entry_id].pages == NULL)
	{
		printf("invalid entry\n");
		mehcached_shm_unlock();
		return false;
	}

	mehcached_shm_entries[entry_id].to_remove = 1;
	mehcached_shm_check_remove(entry_id);

	mehcached_shm_unlock();
	return true;
}

bool
mehcached_shm_map(size_t entry_id, void *ptr, size_t offset, size_t length)
{
	if (((size_t)ptr & ~(mehcached_shm_page_size - 1)) != (size_t)ptr)
	{
		printf("invalid ptr alignment\n");
		return false;
	}

	if ((offset & ~(mehcached_shm_page_size - 1)) != offset)
	{
		printf("invalid offset alignment\n");
		return false;
	}

	mehcached_shm_lock();

	// check entry
	if (mehcached_shm_entries[entry_id].pages == NULL)
	{
		printf("invalid entry\n");
		mehcached_shm_unlock();
		return false;
	}

	if (offset > mehcached_shm_entries[entry_id].length)
	{
		printf("invalid offset\n");
		mehcached_shm_unlock();
		return false;
	}

	if (offset + length > mehcached_shm_entries[entry_id].length)
	{
		printf("invalid length\n");
		mehcached_shm_unlock();
		return false;
	}

	// find empty mapping
	size_t mapping_id;
	for (mapping_id = 0; mapping_id < MEHCACHED_SHM_MAX_MAPPINGS; mapping_id++)
	{
		if (mehcached_shm_mappings[mapping_id].addr == NULL)
			break;
	}

	if (mapping_id == MEHCACHED_SHM_MAX_MAPPINGS)
	{
		printf("too many mappings\n");
		mehcached_shm_unlock();
		return false;
	}

	size_t page_offset = offset / mehcached_shm_page_size; 
	size_t num_pages = (length + (mehcached_shm_page_size - 1)) / mehcached_shm_page_size;

	// map
	void *p = ptr;
	size_t page_index = page_offset;
	size_t page_index_end = page_offset + num_pages;
	int error = 0;
	while (page_index < page_index_end)
	{
		int fd = open(mehcached_shm_pages[mehcached_shm_entries[entry_id].pages[page_index]].path, O_RDWR);
		if (fd == -1)
		{
			perror("");
			error = 1;
			assert(false);
			break;
		}

		void *ret_p = mmap(p, mehcached_shm_page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

		close(fd);

		if (ret_p == MAP_FAILED)
		{
			printf("mmap failed at %p\n", p);
			error = 1;
			break;
		}

		page_index++;
		p = (void *)((size_t)p + mehcached_shm_page_size);
	}
	if (error)
	{
		// clean partialy mapped memory
		p = ptr;
		size_t page_index_clean = page_offset;
		while (page_index_clean < page_index)
		{
			munmap(p, mehcached_shm_page_size);
			page_index_clean++;
			p = (void *)((size_t)p + mehcached_shm_page_size);
		}
		mehcached_shm_unlock();
		return false;
	}

	// register mapping
	if (mehcached_shm_entries[entry_id].refcount == 0)
		mehcached_shm_used_memory += mehcached_shm_entries[entry_id].num_pages * mehcached_shm_page_size;
	mehcached_shm_entries[entry_id].refcount++;

	mehcached_shm_mappings[mapping_id].entry_id = entry_id;
	mehcached_shm_mappings[mapping_id].addr = ptr;
	mehcached_shm_mappings[mapping_id].length = length;
	mehcached_shm_mappings[mapping_id].page_offset = page_offset;
	mehcached_shm_mappings[mapping_id].num_pages = num_pages;

	mehcached_shm_unlock();

#ifndef NDEBUG
	printf("created new mapping %zu (shm entry %zu, page_offset=%zu, num_pages=%zu) at %p\n", mapping_id, entry_id, page_offset, num_pages, ptr);
#endif

	return true;
}

bool
mehcached_shm_unmap(void *ptr)
{
	mehcached_shm_lock();

	// find mapping
	size_t mapping_id;
	for (mapping_id = 0; mapping_id < MEHCACHED_SHM_MAX_MAPPINGS; mapping_id++)
	{
		if (mehcached_shm_mappings[mapping_id].addr == ptr)
			break;
	}

	if (mapping_id == MEHCACHED_SHM_MAX_MAPPINGS)
	{
		printf("invalid unmap\n");
		mehcached_shm_unlock();
		return false;
	}

	// unmap pages
	size_t page_index;
	for (page_index = 0; page_index < mehcached_shm_mappings[mapping_id].num_pages; page_index++)
	{
		munmap(ptr, mehcached_shm_page_size);
		ptr = (void *)((size_t)ptr + mehcached_shm_page_size);
	}

	// remove reference to entry
	--mehcached_shm_entries[mehcached_shm_mappings[mapping_id].entry_id].refcount;
	mehcached_shm_check_remove(mehcached_shm_mappings[mapping_id].entry_id);

	// remove mapping
	memset(&mehcached_shm_mappings[mapping_id], 0, sizeof(mehcached_shm_mappings[mapping_id]));

	mehcached_shm_unlock();

#ifndef NDEBUG
	printf("removed mapping %zu at %p\n", mapping_id, ptr);
#endif

	return true;
}

size_t
mehcached_shm_get_page_size()
{
	return mehcached_shm_page_size;
}

size_t
mehcached_shm_get_memuse()
{
	return mehcached_shm_used_memory;
}

#ifdef USE_DPDK

void *
mehcached_shm_malloc_contiguous(size_t size, size_t lcore)
{
    size = mehcached_shm_adjust_size(size);
    // size_t entry_id = mehcached_shm_alloc(size, (size_t)-1);
    // size_t entry_id = mehcached_shm_alloc(size, numa_node);
    size_t entry_id = mehcached_shm_alloc(size, rte_lcore_to_socket_id((unsigned int)lcore));
    if (entry_id == (size_t)-1)
    	return NULL;
    while (true)
    {
        void *p = mehcached_shm_find_free_address(size);
        if (p == NULL)
        {
            mehcached_shm_schedule_remove(entry_id);
            return NULL;
        }
        if (mehcached_shm_map(entry_id, p, 0, size))
        {
            mehcached_shm_schedule_remove(entry_id);
            return p;
        }
    }
}

void *
mehcached_shm_malloc_contiguous_local(size_t size)
{
    return mehcached_shm_malloc_contiguous(size, rte_lcore_id());
}

void *
mehcached_shm_malloc_contiguous_any(size_t size)
{
    void *p;
    p = mehcached_shm_malloc_contiguous(size, rte_lcore_id());
    if (p == NULL)
        p = mehcached_shm_malloc_contiguous(size, 1 - rte_lcore_id());
    return p;
}

void
mehcached_shm_free_contiguous(void *ptr)
{
    mehcached_shm_unmap(ptr);
}

void *
mehcached_shm_malloc_striped(size_t size)
{
	size += 2 * mehcached_shm_page_size;	// need to store metadata

    size_t size_2 = (size + 1) / 2;
    size_2 = mehcached_shm_adjust_size(size_2);

    // TODO: allocate 1 fewer page (i.e., only 1 page in total) when the total number of pages required is an odd numbered

    size_t entry_id[2];
    entry_id[0] = mehcached_shm_alloc(size_2, rte_socket_id());
    if (entry_id[0] == (size_t)-1)
        return NULL;

    entry_id[1] = mehcached_shm_alloc(size_2, 1 - rte_socket_id());
    if (entry_id[1] == (size_t)-1)
    {
        mehcached_shm_schedule_remove(entry_id[0]);
        return NULL;
    }

    while (true)
    {
        void *base = mehcached_shm_find_free_address(size);
        if (base == NULL)
        {
            mehcached_shm_schedule_remove(entry_id[0]);
            mehcached_shm_schedule_remove(entry_id[1]);
            return NULL;
        }

        void *p = base;
        size_t numa_node = 0;
        size_t offset_in_stripe = 0;
        size_t mapped;
        for (mapped = 0; mapped < size; mapped += mehcached_shm_page_size)
        {
            if (!mehcached_shm_map(entry_id[numa_node], p, offset_in_stripe, mehcached_shm_page_size))
            {
                // error
                break;
            }
            numa_node = 1 - numa_node;
            if (numa_node == 0)
                offset_in_stripe += mehcached_shm_page_size;
            p = (void *)((size_t)p + mehcached_shm_page_size);
        }

        mehcached_shm_schedule_remove(entry_id[0]);
        mehcached_shm_schedule_remove(entry_id[1]);

        if (mapped < size)
        {
            // error
            p = base;
            size_t clean;
            for (clean = 0; clean < mapped; clean += mehcached_shm_page_size)
            {
                mehcached_shm_unmap(p);
                p = (void *)((size_t)p + mehcached_shm_page_size);
            }
        }
        else
        {
            // success
        	*(uint64_t *)base = size;
        	void *ptr = (void *)((size_t)base + 2 * mehcached_shm_page_size);
            return ptr;
        }
    }
}

void
mehcached_shm_free_striped(void *ptr)
{
    void *base = (void *)((size_t)ptr - 2 * mehcached_shm_page_size);
    uint64_t size = *(uint64_t *)base;

    void *p = base;
    size_t mapped;
    for (mapped = 0; mapped < size; mapped += mehcached_shm_page_size)
    {
        mehcached_shm_unmap(p);
        p = (void *)((size_t)p + mehcached_shm_page_size);
    }
}

#endif

MEHCACHED_END


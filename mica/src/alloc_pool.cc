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

#include "alloc_pool.h"
#include "table.h"
#include "shm.h"
#include "util.h"

#include <stdio.h>

MEHCACHED_BEGIN

void
mehcached_pool_init(struct mehcached_pool *alloc, uint64_t size, bool concurrent_alloc_read, bool concurrent_alloc_write, size_t numa_node)
{
    if (size < MEHCACHED_MINIMUM_POOL_SIZE)
        size = MEHCACHED_MINIMUM_POOL_SIZE;
    size = mehcached_shm_adjust_size(size);
    size = mehcached_next_power_of_two(size);
    assert(size <= MEHCACHED_ITEM_OFFSET_MASK >> 1);    // ">> 1" is for sufficient garbage collection time
    assert(size == mehcached_shm_adjust_size(size));

    if (!concurrent_alloc_read)
        alloc->concurrent_access_mode = 0;
    else if (!concurrent_alloc_write)
        alloc->concurrent_access_mode = 1;
    else
        alloc->concurrent_access_mode = 2;

    alloc->size = size;
    alloc->mask = size - 1;

    alloc->lock = 0;
    alloc->head = alloc->tail = 0;

    size_t shm_id = mehcached_shm_alloc(size, numa_node);
    if (shm_id == (size_t)-1)
	{
		printf("failed to allocate memory\n");
        assert(false);
	}
    while (true)
    {
		alloc->data = (uint8_t *) mehcached_shm_find_free_address(size + MEHCACHED_MINIMUM_POOL_SIZE);
		if (alloc->data == NULL)
			assert(false);

        if (!mehcached_shm_map(shm_id, alloc->data, 0, size))
			continue;

		// aliased access across pool end boundary
        if (!mehcached_shm_map(shm_id, alloc->data + size, 0, MEHCACHED_MINIMUM_POOL_SIZE))
        {
            mehcached_shm_unmap(alloc->data);
            continue;
        }

        break;
    }

    if (!mehcached_shm_schedule_remove(shm_id))
    {
        perror("");
        assert(false);
    }
}

void
mehcached_pool_free(struct mehcached_pool *alloc)
{
	if (!mehcached_shm_unmap(alloc->data))
		assert(false);
	if (!mehcached_shm_unmap(alloc->data + alloc->size))
		assert(false);
}

void
mehcached_pool_reset(struct mehcached_pool *alloc)
{
    alloc->head = alloc->tail = 0;
}

void
mehcached_pool_lock(struct mehcached_pool *alloc MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_CONCURRENT
    if (alloc->concurrent_access_mode == 2)
    {
        while (1)
        {
            if (__sync_bool_compare_and_swap((volatile uint32_t *)&alloc->lock, 0U, 1U))
                break;
        }
    }
#endif
}

void
mehcached_pool_unlock(struct mehcached_pool *alloc MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_CONCURRENT
    if (alloc->concurrent_access_mode == 2)
    {
        memory_barrier();
        assert((*(volatile uint32_t *)&alloc->lock & 1U) == 1U);
        // no need to use atomic add because this thread is the only one writing to version
        *(volatile uint32_t *)&alloc->lock = 0U;
    }
#endif
}

struct mehcached_alloc_item *
mehcached_pool_item(const struct mehcached_pool *alloc, uint64_t pool_offset)
{
    return (struct mehcached_alloc_item *)(alloc->data + (pool_offset & alloc->mask));
}

void
mehcached_pool_check_invariants(const struct mehcached_pool *alloc MEHCACHED_UNUSED)
{
    assert(alloc->tail - alloc->head <= alloc->size);
}

void
mehcached_pool_pop_head(struct mehcached_pool *alloc)
{
    struct mehcached_alloc_item *alloc_item = mehcached_pool_item(alloc, alloc->head);
#ifdef MEHCACHED_VERBOSE
    printf("popping item size = %u at head = %lu\n", alloc_item->item_size, alloc->head & MEHCACHED_ITEM_OFFSET_MASK);
#endif

    alloc->head += alloc_item->item_size;
    mehcached_pool_check_invariants(alloc);
}

uint64_t
mehcached_pool_push_tail(struct mehcached_pool *alloc, uint32_t item_size)
{
    assert(item_size == MEHCACHED_ROUNDUP8(item_size));
    assert(item_size <= alloc->size);

    uint64_t item_offset = alloc->tail;

    uint64_t v = item_offset + item_size;
    while (v > alloc->head + alloc->size)
        mehcached_pool_pop_head(alloc);

    struct mehcached_alloc_item *alloc_item = mehcached_pool_item(alloc, item_offset);
    alloc_item->item_size = item_size;

    if (alloc->concurrent_access_mode == 0)
        alloc->tail += item_size;
    else
    {
        *(volatile uint64_t *)&alloc->tail += item_size;
        memory_barrier();
    }

    mehcached_pool_check_invariants(alloc);

#ifdef MEHCACHED_VERBOSE
    printf("pushing item size = %u at tail = %lu\n", item_size, item_offset & MEHCACHED_ITEM_OFFSET_MASK);
#endif

    return item_offset & MEHCACHED_ITEM_OFFSET_MASK;
}

uint64_t
mehcached_pool_allocate(struct mehcached_pool *alloc, uint32_t item_size)
{
    return mehcached_pool_push_tail(alloc, item_size);
}

bool
mehcached_pool_is_valid(const struct mehcached_pool *alloc, uint64_t pool_offset)
{
    if (alloc->concurrent_access_mode == 0)
        return ((alloc->tail - pool_offset) & MEHCACHED_ITEM_OFFSET_MASK) <= alloc->size;
    else
    {
        memory_barrier();
        return ((*(volatile uint64_t *)&alloc->tail - pool_offset) & MEHCACHED_ITEM_OFFSET_MASK) <= alloc->size;
    }
}

MEHCACHED_END


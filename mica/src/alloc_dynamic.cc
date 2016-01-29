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

#include "alloc_dynamic.h"
#include "table.h"
#include "shm.h"
#include "util.h"
#include <stdio.h>

MEHCACHED_BEGIN

#define MEHCACHED_DYNAMIC_FREE (0UL)
#define MEHCACHED_DYNAMIC_OCCUPIED (1UL)

#define MEHCACHED_DYNAMIC_TAG_SIZE(vec) ((vec) & ((1UL << 63UL) - 1UL))
#define MEHCACHED_DYNAMIC_TAG_STATUS(vec) ((vec) >> 63UL)
#define MEHCACHED_DYNAMIC_TAG_VEC(size, status) ((size) | (status) << 63UL)

// TODO: use address order for each freelist to reduce fragmentation and improve locality
// TODO: use the LSB (not MSB) to store status as all sizes are aligned to 8-byte boundary

static
void
mehcached_dynamic_init(struct mehcached_dynamic *alloc, uint64_t size, bool concurrent_alloc_read, bool concurrent_alloc_write, size_t numa_node)
{
    if (!concurrent_alloc_read)
        alloc->concurrent_access_mode = 0;
    else if (!concurrent_alloc_write)
        alloc->concurrent_access_mode = 1;
    else
        alloc->concurrent_access_mode = 2;

    alloc->lock = 0;

    size = mehcached_shm_adjust_size(size);
    assert(size <= MEHCACHED_DYNAMIC_MAX_SIZE);

    alloc->size = size;

    size_t shm_id = mehcached_shm_alloc(size, numa_node);
    if (shm_id == (size_t)-1)
    {
        printf("failed to allocate memory\n");
        assert(false);
    }
    while (true)
    {
        alloc->data = (uint8_t *) mehcached_shm_find_free_address(size);
        if (alloc->data == NULL)
            assert(false);

        if (!mehcached_shm_map(shm_id, alloc->data, 0, size))
            continue;

        break;
    }

    if (!mehcached_shm_schedule_remove(shm_id))
    {
        perror("");
        assert(false);
    }

    mehcached_dynamic_reset(alloc);
}

static
void
mehcached_dynamic_free(struct mehcached_dynamic *alloc)
{
    if (!mehcached_shm_unmap(alloc->data))
        assert(false);
}

static
void
mehcached_dynamic_lock(struct mehcached_dynamic *alloc MEHCACHED_UNUSED)
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

static
void
mehcached_dynamic_unlock(struct mehcached_dynamic *alloc MEHCACHED_UNUSED)
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

static
size_t
mehcached_dynamic_size_to_class_roundup(uint64_t size)
{
    assert(size <= MEHCACHED_DYNAMIC_MAX_SIZE);

    if (size <= MEHCACHED_DYNAMIC_MIN_SIZE + (MEHCACHED_DYNAMIC_NUM_CLASSES - 1) * MEHCACHED_DYNAMIC_CLASS_INCREMENT)
        return (size - MEHCACHED_DYNAMIC_MIN_SIZE + MEHCACHED_DYNAMIC_CLASS_INCREMENT - 1) / MEHCACHED_DYNAMIC_CLASS_INCREMENT;
    else
        return MEHCACHED_DYNAMIC_NUM_CLASSES - 1;
}

static
size_t
mehcached_dynamic_size_to_class_rounddown(uint64_t size)
{
    assert(size <= MEHCACHED_DYNAMIC_MAX_SIZE);
	assert(size >= MEHCACHED_DYNAMIC_MIN_SIZE);

    if (size < MEHCACHED_DYNAMIC_MIN_SIZE + MEHCACHED_DYNAMIC_NUM_CLASSES * MEHCACHED_DYNAMIC_CLASS_INCREMENT)
        return (size - MEHCACHED_DYNAMIC_MIN_SIZE) / MEHCACHED_DYNAMIC_CLASS_INCREMENT;
    else
        return MEHCACHED_DYNAMIC_NUM_CLASSES - 1;
}

static
void
mehcached_dynamic_insert_free_chunk(struct mehcached_dynamic *alloc, uint8_t *chunk_start, uint64_t chunk_size)
{
#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_insert_free_chunk: start=%p size=%lu\n", chunk_start, chunk_size);
#endif
    size_t chunk_class = mehcached_dynamic_size_to_class_rounddown(chunk_size);
    *(uint64_t *)chunk_start = *(uint64_t *)(chunk_start + chunk_size - 8) = MEHCACHED_DYNAMIC_TAG_VEC(chunk_size, MEHCACHED_DYNAMIC_FREE);
    *(uint8_t **)(chunk_start + 8) = NULL;  // the head has no previous free chunk
    *(uint8_t **)(chunk_start + 16) = alloc->free_head[chunk_class];    // point to the old head

    if (alloc->free_head[chunk_class] != NULL)
    {
        assert(*(uint8_t **)(alloc->free_head[chunk_class] + 8) == NULL);
        *(uint8_t **)(alloc->free_head[chunk_class] + 8) = chunk_start; // update the previous head's prev pointer
    }

    alloc->free_head[chunk_class] = chunk_start;    // set as a new head
}

static
void mehcached_dynamic_remove_free_chunk_from_free_list(struct mehcached_dynamic *alloc, uint8_t *chunk_start, uint64_t chunk_size)
{
#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_remove_free_chunk_from_free_list: start=%p size=%lu\n", chunk_start, chunk_size);
#endif

    uint8_t *prev_chunk_start = *(uint8_t **)(chunk_start + 8);
    uint8_t *next_chunk_start = *(uint8_t **)(chunk_start + 16);

    if (prev_chunk_start != NULL)
        *(uint8_t **)(prev_chunk_start + 16) = next_chunk_start;
    else
    {
        size_t chunk_class = mehcached_dynamic_size_to_class_rounddown(chunk_size);
        assert(alloc->free_head[chunk_class] == chunk_start);
        alloc->free_head[chunk_class] = next_chunk_start;        // set the next free chunk as the head
    }

    if (next_chunk_start != NULL)
        *(uint8_t **)(next_chunk_start + 8) = prev_chunk_start;
}

static
bool
mehcached_dynamic_remove_free_chunk_from_head(struct mehcached_dynamic *alloc, uint64_t minimum_chunk_size, uint8_t **out_chunk_start, uint64_t *out_chunk_size)
{
    size_t chunk_class = mehcached_dynamic_size_to_class_roundup(minimum_chunk_size);

    // determine the size class to use (best fit)
    for (; chunk_class < MEHCACHED_DYNAMIC_NUM_CLASSES; chunk_class++)
        if (alloc->free_head[chunk_class] != NULL)
            break;

    if (chunk_class == MEHCACHED_DYNAMIC_NUM_CLASSES)
    {
#ifdef MEHCACHED_VERBOSE
        printf("mehcached_dynamic_remove_free_chunk_from_head: minsize=%lu no space\n", minimum_chunk_size);
#endif
        return false;
    }

    // use the first free chunk in the class; the overall policy is still approximately best fit (which is good) due to segregation
    uint8_t *chunk_start = alloc->free_head[chunk_class];
    assert(MEHCACHED_DYNAMIC_TAG_STATUS(*(uint64_t *)chunk_start) == MEHCACHED_DYNAMIC_FREE);
    uint64_t chunk_size = MEHCACHED_DYNAMIC_TAG_SIZE(*(uint64_t *)chunk_start);
    assert(*(uint64_t *)chunk_start == *(uint64_t *)(chunk_start + chunk_size - 8));

    assert(chunk_size >= minimum_chunk_size);

    mehcached_dynamic_remove_free_chunk_from_free_list(alloc, chunk_start, chunk_size);

    *out_chunk_start = chunk_start;
    *out_chunk_size = chunk_size;
#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_remove_free_chunk_from_head: minsize=%lu start=%p size=%lu\n", minimum_chunk_size, *out_chunk_start, *out_chunk_size);
#endif
    return true;
}

static
void
mehcached_dynamic_reset(struct mehcached_dynamic *alloc)
{
    memset(alloc->free_head, 0, sizeof(alloc->free_head));

    // set the entire free space as a free chunk
    mehcached_dynamic_insert_free_chunk(alloc, alloc->data, alloc->size);
}

static
struct mehcached_alloc_item *
mehcached_dynamic_item(const struct mehcached_dynamic *alloc, uint64_t dynamic_offset)
{
    return (struct mehcached_alloc_item *)(alloc->data + dynamic_offset);
}

static
void
mehcached_dynamic_coalese_free_chunk_left(struct mehcached_dynamic *alloc, uint8_t **chunk_start, uint64_t *chunk_size)
{
    if (*chunk_start == alloc->data)
        return;
    assert(*chunk_start > alloc->data);

    if (MEHCACHED_DYNAMIC_TAG_STATUS(*(uint64_t *)(*chunk_start - 8)) == MEHCACHED_DYNAMIC_OCCUPIED)
        return;

    uint64_t adj_chunk_size = MEHCACHED_DYNAMIC_TAG_SIZE(*(uint64_t *)(*chunk_start - 8));
    uint8_t *adj_chunk_start = *chunk_start - adj_chunk_size;
    assert(*(uint64_t *)adj_chunk_start == *(uint64_t *)(adj_chunk_start + adj_chunk_size - 8));

#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_coalese_free_chunk_left: start=%p size=%lu left=%lu\n", *chunk_start, *chunk_size, adj_chunk_size);
#endif

    mehcached_dynamic_remove_free_chunk_from_free_list(alloc, adj_chunk_start, adj_chunk_size);
    *chunk_start = adj_chunk_start;
    *chunk_size = *chunk_size + adj_chunk_size;
}

static
void
mehcached_dynamic_coalese_free_chunk_right(struct mehcached_dynamic *alloc, uint8_t **chunk_start, uint64_t *chunk_size)
{
    if (*chunk_start + *chunk_size == alloc->data + alloc->size)
        return;
    assert(*chunk_start + *chunk_size < alloc->data + alloc->size);

    if (MEHCACHED_DYNAMIC_TAG_STATUS(*(uint64_t *)(*chunk_start + *chunk_size)) == MEHCACHED_DYNAMIC_OCCUPIED)
        return;

    uint8_t *adj_chunk_start = *chunk_start + *chunk_size;
    uint64_t adj_chunk_size = MEHCACHED_DYNAMIC_TAG_SIZE(*(uint64_t *)adj_chunk_start);
    assert(*(uint64_t *)adj_chunk_start == *(uint64_t *)(adj_chunk_start + adj_chunk_size - 8));

#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_coalese_free_chunk_right: start=%p size=%lu right=%lu\n", *chunk_start, *chunk_size, adj_chunk_size);
#endif

    mehcached_dynamic_remove_free_chunk_from_free_list(alloc, adj_chunk_start, adj_chunk_size);
    // chunk_start is unchanged
    *chunk_size = *chunk_size + adj_chunk_size;
}

static
uint64_t
mehcached_dynamic_allocate(struct mehcached_dynamic *alloc, uint32_t item_size)
{
    uint64_t minimum_chunk_size = MEHCACHED_ROUNDUP8((uint64_t)item_size) + MEHCAHCED_DYNAMIC_OVERHEAD;

    uint8_t *chunk_start;
    uint64_t chunk_size;
    if (!mehcached_dynamic_remove_free_chunk_from_head(alloc, minimum_chunk_size, &chunk_start, &chunk_size))
        return MEHCACHED_DYNAMIC_INSUFFICIENT_SPACE;

    // see if we can make a leftover free chunk
    uint64_t leftover_chunk_size = chunk_size - minimum_chunk_size;
    if (leftover_chunk_size >= MEHCACHED_DYNAMIC_MIN_SIZE)
    {
        // create a leftover free chunk and insert it to the freelist
        mehcached_dynamic_insert_free_chunk(alloc, chunk_start + minimum_chunk_size, leftover_chunk_size);
        // coalescing is not required here because the previous chunk already used to be a big coalesced free chunk

        // adjust the free chunk to avoid overlapping
        chunk_size = minimum_chunk_size;
    }
    else
        leftover_chunk_size = 0;

#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_allocate: item_size=%u minsize=%lu start=%p size=%lu (leftover=%lu)\n", item_size, minimum_chunk_size, chunk_start, chunk_size, leftover_chunk_size);
#endif

    *(uint64_t *)chunk_start = *(uint64_t *)(chunk_start + chunk_size - 8) = MEHCACHED_DYNAMIC_TAG_VEC(chunk_size, MEHCACHED_DYNAMIC_OCCUPIED);

    // TODO: We are wasting 4 bytes for struct mehcached_alloc_item for compatibility.  Need to implement an allocator-specific method to obtain the item size
    struct mehcached_alloc_item *alloc_item = (struct mehcached_alloc_item *)(chunk_start + 8);
    alloc_item->item_size = item_size;

    return (uint64_t)((uint8_t *)alloc_item - alloc->data);
}

static
void
mehcached_dynamic_deallocate(struct mehcached_dynamic *alloc, uint64_t dynamic_offset)
{
    struct mehcached_alloc_item *alloc_item = mehcached_dynamic_item(alloc, dynamic_offset);
    uint8_t *chunk_start = (uint8_t *)alloc_item - 8;
    assert(MEHCACHED_DYNAMIC_TAG_STATUS(*(uint64_t *)chunk_start) == MEHCACHED_DYNAMIC_OCCUPIED);
    uint64_t chunk_size = MEHCACHED_DYNAMIC_TAG_SIZE(*(uint64_t *)chunk_start);

#ifdef MEHCACHED_VERBOSE
    printf("mehcached_dynamic_deallocate: start=%p size=%lu\n", chunk_start, chunk_size);
#endif

    mehcached_dynamic_coalese_free_chunk_left(alloc, &chunk_start, &chunk_size);
    mehcached_dynamic_coalese_free_chunk_right(alloc, &chunk_start, &chunk_size);
    mehcached_dynamic_insert_free_chunk(alloc, chunk_start, chunk_size);
}

MEHCACHED_END

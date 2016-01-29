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

#ifndef ALLOC_POOL_H_
#define ALLOC_POOL_H_

#include "common.h"
#include "alloc.h"

MEHCACHED_BEGIN

// the minimum pool size that will prevent any invalid read with garbage item metadata
// this must be at least as large as the rounded sum of an item header, key, and value,
// and must also be a multiple of mehcached_shm_get_page_size()
#define MEHCACHED_MINIMUM_POOL_SIZE (2097152)

struct mehcached_pool
{
    uint8_t concurrent_access_mode;
    uint32_t lock;
    uint8_t *data;
    uint64_t size;  // a power of two
    uint64_t mask;  // size - 1; this mask is used only when converting the offset to the actual location of the item
    // internally, pool uses full 64-bit numbers for head and tail
    // however, the valid range for item_offset is limited to (MEHCACHED_ITEM_OFFSET_MASK + 1)
    // we resolve this inconsistency by applying MEHCACHED_ITEM_OFFSET_MASK mask
    // whenever returning the offset to the outside or using a masked offset given from the outside
    uint64_t head;  // start offset of items
    uint64_t tail;  // end offset of items
} MEHCACHED_ALIGNED(64);

void
mehcached_pool_init(struct mehcached_pool *alloc, uint64_t size, bool concurrent_alloc_read, bool concurrent_alloc_write, size_t numa_node);

void
mehcached_pool_free(struct mehcached_pool *alloc);

void
mehcached_pool_reset(struct mehcached_pool *alloc);

void
mehcached_pool_lock(struct mehcached_pool *alloc);

void
mehcached_pool_unlock(struct mehcached_pool *alloc);

struct mehcached_alloc_item *
mehcached_pool_item(const struct mehcached_pool *alloc, uint64_t pool_offset);

void
mehcached_pool_check_invariants(const struct mehcached_pool *alloc);

void
mehcached_pool_pop_head(struct mehcached_pool *alloc);

uint64_t
mehcached_pool_push_tail(struct mehcached_pool *alloc, uint32_t item_size);

uint64_t
mehcached_pool_allocate(struct mehcached_pool *alloc, uint32_t item_size);

bool
mehcached_pool_is_valid(const struct mehcached_pool *alloc, uint64_t pool_offset);

MEHCACHED_END

#endif // ALLOC_POOL_H_

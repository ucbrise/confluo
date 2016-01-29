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

#include "alloc_malloc.h"
#include "table.h"

#include <stdio.h>

MEHCACHED_BEGIN

static
void
mehcached_malloc_init(struct mehcached_malloc *alloc)
{
    uint8_t *ptr = (uint8_t *)malloc(8);
    free(ptr);
    alloc->pointer_base = ptr - 0x7fffffffUL;
}

static
void
mehcached_malloc_free(struct mehcached_malloc *alloc MEHCACHED_UNUSED)
{
}

static
void
mehcached_malloc_reset(struct mehcached_malloc *alloc MEHCACHED_UNUSED)
{
}

static
struct mehcached_alloc_item *
mehcached_malloc_item(const struct mehcached_malloc *alloc, uint64_t malloc_offset)
{
    return (struct mehcached_alloc_item *)(alloc->pointer_base + malloc_offset);
}

static
uint64_t
mehcached_malloc_allocate(struct mehcached_malloc *alloc, uint32_t item_size)
{
    void *p = malloc(item_size);
    if (p == NULL)
        return MEHCACHED_MALLOC_INSUFFICIENT_SPACE;

    size_t malloc_offset = (size_t)((uint8_t *)p - alloc->pointer_base);
    if (malloc_offset > MEHCACHED_ITEM_OFFSET_MASK)
    {
        printf("too large pointer: %zx (offset = %zx)\n", (size_t)p, malloc_offset);
        assert(false);
        return MEHCACHED_MALLOC_INSUFFICIENT_SPACE;
    }
    struct mehcached_alloc_item *alloc_item = (struct mehcached_alloc_item *)p;
    alloc_item->item_size = item_size;
    return (uint64_t)malloc_offset;
}

static
void
mehcached_malloc_deallocate(struct mehcached_malloc *alloc, uint64_t malloc_offset)
{
    free(alloc->pointer_base + malloc_offset);
}

MEHCACHED_END


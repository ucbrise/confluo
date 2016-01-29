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

#ifndef ALLOC_MALLOC_H_
#define ALLOC_MALLOC_H_

#include "common.h"
#include "alloc.h"

MEHCACHED_BEGIN

#define MEHCACHED_MALLOC_INSUFFICIENT_SPACE ((uint64_t)-1)

struct mehcached_malloc
{
    uint8_t *pointer_base;
};

static
void
mehcached_malloc_init(struct mehcached_malloc *alloc);

static
void
mehcached_malloc_free(struct mehcached_malloc *alloc);

static
void
mehcached_malloc_reset(struct mehcached_malloc *alloc);

static
struct mehcached_alloc_item *
mehcached_malloc_item(const struct mehcached_malloc *alloc, uint64_t malloc_offset);

static
uint64_t
mehcached_malloc_allocate(struct mehcached_malloc *alloc, uint32_t item_size);

static
void
mehcached_malloc_deallocate(struct mehcached_malloc *alloc, uint64_t malloc_offset);

MEHCACHED_END

#endif // ALLOC_MALLOC_H_

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

#ifndef ALLOC_DYNAMIC_H_
#define ALLOC_DYNAMIC_H_

#include "common.h"
#include "alloc.h"

MEHCACHED_BEGIN

// memory allocation using segregated fit (similar to Lea)

#define MEHCAHCED_DYNAMIC_OVERHEAD (16)	// per-item space overhead solely caused by mehcached_dynamic

#define MEHCACHED_DYNAMIC_MIN_SIZE (32UL)				// 32 bytes (must be able to hold 4 size_t variables)
#define MEHCACHED_DYNAMIC_MAX_SIZE ((1UL << 40) - 1)	// 40-bit wide size (can be up to 63-bit wide)
#define MEHCACHED_DYNAMIC_NUM_CLASSES (32)		// 32 classes for freelists
#define MEHCACHED_DYNAMIC_CLASS_INCREMENT (8)	// 8-byte increment in freelist classes

#define MEHCACHED_DYNAMIC_INSUFFICIENT_SPACE ((uint64_t)-1)

// data structure layout

// free_head[class] -> the first free chunk of the class (NULL if none exists)

// free chunk (of size N) - N is the same or larger than the size of the class
// 8-byte: status (1 bit), size (63 bit)
// 8-byte: prev free chunk of the same class (NULL if head)
// 8-byte: next free chunk of the same class (NULL if tail)
// (N - 32 bytes)
// 8-byte: status (1 bit), size (63 bit)

// occupied chunk (of size N) - overhead of 16 bytes
// 8-byte: status (1 bit), size (63 bit)
// (N - 16 bytes)
// 8-byte: status (1 bit), size (63 bit)

struct mehcached_dynamic
{
    uint8_t concurrent_access_mode;
    uint32_t lock;
	uint64_t size;	// the total size
    uint8_t *data;	// the base address of the reserved memory
    uint8_t *free_head[MEHCACHED_DYNAMIC_NUM_CLASSES];	// the head free pointer of each class
};

static
void
mehcached_dynamic_init(struct mehcached_dynamic *alloc, uint64_t size, bool concurrent_alloc_read, bool concurrent_alloc_write, size_t numa_node);

static
void
mehcached_dynamic_free(struct mehcached_dynamic *alloc);

static
void
mehcached_dynamic_reset(struct mehcached_dynamic *alloc);

static
void
mehcached_dynamic_lock(struct mehcached_dynamic *alloc);

static
void
mehcached_dynamic_unlock(struct mehcached_dynamic *alloc);

static
struct mehcached_alloc_item *
mehcached_dynamic_item(const struct mehcached_dynamic *alloc, uint64_t dynamic_offset);

static
uint64_t
mehcached_dynamic_allocate(struct mehcached_dynamic *alloc, uint32_t item_size);

static
void
mehcached_dynamic_deallocate(struct mehcached_dynamic *alloc, uint64_t dynamic_offset);

MEHCACHED_END

#endif // ALLOC_DYNAMIC_H_

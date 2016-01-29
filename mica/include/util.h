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

#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"
#include "shm.h"

#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/resource.h>

#ifdef USE_DPDK
#include <rte_memcpy.h>
#endif

#define MEHCACHED_ROUNDUP8(x) (((x) + 7UL) & (~7UL))
#define MEHCACHED_ROUNDUP64(x) (((x) + 63UL) & (~63UL))
#define MEHCACHED_ROUNDUP4K(x) (((x) + 4095UL) & (~4095UL))
#define MEHCACHED_ROUNDUP1M(x) (((x) + 1048575UL) & (~1048575UL))
#define MEHCACHED_ROUNDUP2M(x) (((x) + 2097151UL) & (~2097151UL))

MEHCACHED_BEGIN

static
size_t
mehcached_next_power_of_two(size_t v)
{
    size_t s = 0;
    while (((size_t)1 << s) < v)
        s++;
    return (size_t)1 << s;
}

static
void
memory_barrier()
{
    asm volatile("" ::: "memory");
}

static
void
mehcached_memcpy8(uint8_t *dest, const uint8_t *src, size_t length)
{
    length = MEHCACHED_ROUNDUP8(length);
#ifndef USE_DPDK
    switch (length >> 3)
    {
        case 0:
            break;
        case 1:
            *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
            break;
        case 2:
            *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
            *(uint64_t *)(dest + 8) = *(const uint64_t *)(src + 8);
            break;
        case 3:
            *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
            *(uint64_t *)(dest + 8) = *(const uint64_t *)(src + 8);
            *(uint64_t *)(dest + 16) = *(const uint64_t *)(src + 16);
            break;
        case 4:
            *(uint64_t *)(dest + 0) = *(const uint64_t *)(src + 0);
            *(uint64_t *)(dest + 8) = *(const uint64_t *)(src + 8);
            *(uint64_t *)(dest + 16) = *(const uint64_t *)(src + 16);
            *(uint64_t *)(dest + 24) = *(const uint64_t *)(src + 24);
            break;
        default:
            memcpy(dest, src, length);
            break;
    }
#else
    rte_memcpy(dest, src, length);
#endif
}

static
bool
mehcached_memcmp8(const uint8_t *dest, const uint8_t *src, size_t length)
{
    length = MEHCACHED_ROUNDUP8(length);
    switch (length >> 3)
    {
        case 0:
            return true;
        case 1:
            if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0))
                return false;
            return true;
        case 2:
            if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0))
                return false;
            if (*(const uint64_t *)(dest + 8) != *(const uint64_t *)(src + 8))
                return false;
            return true;
        case 3:
            if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0))
                return false;
            if (*(const uint64_t *)(dest + 8) != *(const uint64_t *)(src + 8))
                return false;
            if (*(const uint64_t *)(dest + 16) != *(const uint64_t *)(src + 16))
                return false;
            return true;
        case 4:
            if (*(const uint64_t *)(dest + 0) != *(const uint64_t *)(src + 0))
                return false;
            if (*(const uint64_t *)(dest + 8) != *(const uint64_t *)(src + 8))
                return false;
            if (*(const uint64_t *)(dest + 16) != *(const uint64_t *)(src + 16))
                return false;
            if (*(const uint64_t *)(dest + 24) != *(const uint64_t *)(src + 24))
                return false;
            return true;
        default:
            return memcmp(dest, src, length) == 0;
    }
}

static
uint32_t
mehcached_rand(uint64_t *state)
{
    // same as Java's
    *state = (*state * 0x5deece66dUL + 0xbUL) & ((1UL << 48) - 1);
    return (uint32_t)(*state >> (48 - 32));
}

static
double
mehcached_rand_d(uint64_t *state)
{
    // caution: this is maybe too non-random
    *state = (*state * 0x5deece66dUL + 0xbUL) & ((1UL << 48) - 1);
    return (double)*state / (double)((1UL << 48) - 1);
}

static
size_t
mehcached_get_memuse()
{
    struct rusage usage;
    int ret = getrusage(RUSAGE_SELF, &usage);
    assert(ret == 0);
    (void)ret;
    return (size_t)usage.ru_maxrss * 1024 + mehcached_shm_get_memuse();
}

#ifdef USE_DPDK

#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_debug.h>

// use this for EAL-related memory allocation only (use mehcached_shm_malloc* instead for other cases)
struct mehcached_eal_malloc_arg
{
    size_t size;
    void *ret;
};

static
int
mehcached_eal_malloc_lcore_internal(void *arg)
{
    struct mehcached_eal_malloc_arg *malloc_arg = (struct mehcached_eal_malloc_arg *)arg;
    malloc_arg->ret = rte_malloc(NULL, malloc_arg->size, 0);
    return 0;
}

static
void *
mehcached_eal_malloc_lcore(size_t size, size_t lcore)
{
    struct mehcached_eal_malloc_arg malloc_arg;
    malloc_arg.size = size;
    if (lcore == rte_lcore_id())
        mehcached_eal_malloc_lcore_internal(&malloc_arg);
    else
    {
        assert(rte_lcore_id() == rte_get_master_lcore());
        rte_eal_remote_launch(mehcached_eal_malloc_lcore_internal, &malloc_arg, (unsigned int)lcore);
        rte_eal_mp_wait_lcore();
    }
    return malloc_arg.ret;
}

static
void
rte_eal_launch(lcore_function_t *f, void *arg, unsigned int core_id)
{
    if (core_id == rte_lcore_id())
        f(arg);
    else
        rte_eal_remote_launch(f, arg, core_id);
}

#endif

MEHCACHED_END

#endif // UTIL_H_

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

#include "table.h"
#include "util.h"
#include "shm.h"

#include <stdio.h>

MEHCACHED_BEGIN

// a test feature to deduplicate PUT requests within the same batch
//#define MEHCACHED_DEDUP_WITHIN_BATCH

void
mehcached_print_bucket(const struct mehcached_bucket *bucket)
{
    printf("<bucket %zx>\n", (size_t)bucket);
    size_t item_index;
    for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        printf("  item_vec[%zu]: tag=%lu, alloc_id=%lu, item_offset=%lu\n", item_index, MEHCACHED_TAG(bucket->item_vec[item_index]), MEHCACHED_ALLOC_ID(bucket->item_vec[item_index]), MEHCACHED_ITEM_OFFSET(bucket->item_vec[item_index]));
}

void
mehcached_print_buckets(const struct mehcached_table *table)
{
    size_t bucket_index;
    for (bucket_index = 0; bucket_index < table->num_buckets; bucket_index++)
    {
        struct mehcached_bucket *bucket = table->buckets + bucket_index;
        mehcached_print_bucket(bucket);
    }
    printf("\n");
}

void
mehcached_print_stats(const struct mehcached_table *table MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_COLLECT_STATS
    printf("count:                  %10zu\n", table->stats.count);
    printf("set_nooverwrite:        %10zu | ", table->stats.set_nooverwrite);
    printf("set_new:                %10zu | ", table->stats.set_new);
    printf("set_inplace:            %10zu | ", table->stats.set_inplace);
    printf("set_evicted:            %10zu\n", table->stats.set_evicted);
    printf("get_found:              %10zu | ", table->stats.get_found);
    printf("get_notfound:           %10zu\n", table->stats.get_notfound);
    printf("test_found:             %10zu | ", table->stats.test_found);
    printf("test_notfound:          %10zu\n", table->stats.test_notfound);
    printf("cleanup:                %10zu\n", table->stats.cleanup);
    printf("move_to_head_performed: %10zu | ", table->stats.move_to_head_performed);
    printf("move_to_head_skipped:   %10zu | ", table->stats.move_to_head_skipped);
    printf("move_to_head_failed:    %10zu\n", table->stats.move_to_head_failed);
#endif
}

void
mehcached_reset_table_stats(struct mehcached_table *table MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_COLLECT_STATS
    size_t count = table->stats.count;
    memset(&table->stats, 0, sizeof(table->stats));
    table->stats.count = count;
#endif
}

uint32_t
mehcached_calc_bucket_index(const struct mehcached_table *table, uint64_t key_hash)
{
    // 16 is from MEHCACHED_TAG_MASK's log length
    return (uint32_t)(key_hash >> 16) & table->num_buckets_mask;
}

uint16_t
mehcached_calc_tag(uint64_t key_hash)
{
    uint16_t tag = (uint16_t)(key_hash & MEHCACHED_TAG_MASK);
    if (tag == 0)
        return 1;
    else
        return tag;
}

// uint64_t
uint32_t
mehcached_read_version_begin(const struct mehcached_table *table MEHCACHED_UNUSED, const struct mehcached_bucket *bucket MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_CONCURRENT
    if (table->concurrent_access_mode != 0)
    {
        while (true)
        {
            // uint64_t v = *(volatile uint64_t *)&bucket->version;
            uint32_t v = *(volatile uint32_t *)&bucket->version;
            memory_barrier();
            // if ((v & 1UL) != 0UL)
            if ((v & 1U) != 0U)
                continue;
            return v;
        }
    }
    else
        // return 0UL;
        return 0U;
#else
    // return 0UL;
    return 0U;
#endif
}

//uint64_t
uint32_t
mehcached_read_version_end(const struct mehcached_table *table MEHCACHED_UNUSED, const struct mehcached_bucket *bucket MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_CONCURRENT
    if (table->concurrent_access_mode != 0)
    {
        memory_barrier();
        // uint64_t v = *(volatile uint64_t *)&bucket->version;
        uint32_t v = *(volatile uint32_t *)&bucket->version;
        return v;
    }
    else
        // return 0UL;
        return 0U;
#else
    // return 0UL;
    return 0U;
#endif
}

void
mehcached_lock_bucket(const struct mehcached_table *table MEHCACHED_UNUSED, struct mehcached_bucket *bucket MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_CONCURRENT
    if (table->concurrent_access_mode == 1)
    {
        // assert((*(volatile uint64_t *)&bucket->version & 1UL) == 0UL);
        // (*(volatile uint64_t *)&bucket->version)++;
        assert((*(volatile uint32_t *)&bucket->version & 1U) == 0U);
        (*(volatile uint32_t *)&bucket->version)++;
        memory_barrier();
    }
    else if (table->concurrent_access_mode == 2)
    {
        while (1)
        {
            // uint64_t v = *(volatile uint64_t *)&bucket->version & ~1UL;
            uint32_t v = *(volatile uint32_t *)&bucket->version & ~1U;
            // uint64_t new_v = v | 1UL;
            uint32_t new_v = v | 1U;
            // if (__sync_bool_compare_and_swap((volatile uint64_t *)&bucket->version, v, new_v))
            if (__sync_bool_compare_and_swap((volatile uint32_t *)&bucket->version, v, new_v))
                break;
        }
    }
#endif
}

void
mehcached_unlock_bucket(const struct mehcached_table *table MEHCACHED_UNUSED, struct mehcached_bucket *bucket MEHCACHED_UNUSED)
{
#ifdef MEHCACHED_CONCURRENT
    if (table->concurrent_access_mode != 0)
    {
        memory_barrier();
        // assert((*(volatile uint64_t *)&bucket->version & 1UL) == 1UL);
        assert((*(volatile uint32_t *)&bucket->version & 1U) == 1U);
        // no need to use atomic add because this thread is the only one writing to version
        // (*(volatile uint64_t *)&bucket->version)++;
        (*(volatile uint32_t *)&bucket->version)++;
    }
#endif
}

void
mehcached_lock_extra_bucket_free_list(struct mehcached_table *table)
{
#ifdef MEHCACHED_CONCURRENT
    if (table->concurrent_access_mode == 2)
    {
        while (1)
        {
            if (__sync_bool_compare_and_swap((volatile uint32_t *)&table->extra_bucket_free_list.lock, 0U, 1U))
                break;
        }
    }
#endif
}

void
mehcached_unlock_extra_bucket_free_list(struct mehcached_table *table)
{
#ifdef MEHCACHED_CONCURRENT
    if (table->concurrent_access_mode == 2)
    {
        memory_barrier();
        assert((*(volatile uint32_t *)&table->extra_bucket_free_list.lock & 1U) == 1U);
        // no need to use atomic add because this thread is the only one writing to version
        *(volatile uint32_t *)&table->extra_bucket_free_list.lock = 0U;
    }
#endif
}

bool
mehcached_has_extra_bucket(struct mehcached_bucket *bucket MEHCACHED_UNUSED)
{
#ifndef MEHCACHED_NO_EVICTION
    return false;
#else
    return bucket->next_extra_bucket_index != 0;
#endif
}

struct mehcached_bucket *
mehcached_extra_bucket(const struct mehcached_table *table, uint32_t extra_bucket_index)
{
    // extra_bucket_index is 1-base
    assert(extra_bucket_index != 0);
    assert(extra_bucket_index < 1 + table->num_extra_buckets);
    return table->extra_buckets + extra_bucket_index;   // extra_buckets[1] is the actual start
}

bool
mehcached_alloc_extra_bucket(struct mehcached_table *table, struct mehcached_bucket *bucket)
{
    assert(!mehcached_has_extra_bucket(bucket));

    mehcached_lock_extra_bucket_free_list(table);

    if (table->extra_bucket_free_list.head == 0)
    {
        mehcached_unlock_extra_bucket_free_list(table);
        return false;
    }

    // take the first free extra bucket
    uint32_t extra_bucket_index = table->extra_bucket_free_list.head;
    table->extra_bucket_free_list.head = mehcached_extra_bucket(table, extra_bucket_index)->next_extra_bucket_index;
    mehcached_extra_bucket(table, extra_bucket_index)->next_extra_bucket_index = 0;

    // add it to the given bucket
    // concurrent readers may see the new extra_bucket from this point
    bucket->next_extra_bucket_index = extra_bucket_index;

    mehcached_unlock_extra_bucket_free_list(table);
    return true;
}

void
mehcached_free_extra_bucket(struct mehcached_table *table, struct mehcached_bucket *bucket)
{
    assert(mehcached_has_extra_bucket(bucket));

    uint32_t extra_bucket_index = bucket->next_extra_bucket_index;

    struct mehcached_bucket *extra_bucket = mehcached_extra_bucket(table, extra_bucket_index);
    assert(extra_bucket->next_extra_bucket_index == 0); // only allows freeing the tail of the extra bucket chain

    // verify if the extra bucket is empty (debug only)
    size_t item_index;
    for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        assert(extra_bucket->item_vec[item_index] == 0);

    // detach
    bucket->next_extra_bucket_index = 0;

    // add freed extra bucket to the free list
    mehcached_lock_extra_bucket_free_list(table);

    extra_bucket->next_extra_bucket_index = table->extra_bucket_free_list.head;
    table->extra_bucket_free_list.head = extra_bucket_index;

    mehcached_unlock_extra_bucket_free_list(table);
}

void
mehcached_fill_hole(struct mehcached_table *table, struct mehcached_bucket *bucket, size_t unused_item_index)
{
    // there is no extra bucket; do not try to fill a hole within the same bucket
    if (!mehcached_has_extra_bucket(bucket))
        return;

    struct mehcached_bucket *prev_extra_bucket = NULL;
    struct mehcached_bucket *current_extra_bucket = bucket;
    while (mehcached_has_extra_bucket(current_extra_bucket) != 0)
    {
        prev_extra_bucket = current_extra_bucket;
        current_extra_bucket = mehcached_extra_bucket(table, current_extra_bucket->next_extra_bucket_index);
    }

    bool last_item = true;
    size_t moved_item_index;

    size_t item_index;
    for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        if (current_extra_bucket->item_vec[item_index] != 0)
        {
            moved_item_index = item_index;
            break;
        }

    for (item_index++; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        if (current_extra_bucket->item_vec[item_index] != 0)
        {
            last_item = false;
            break;
        }

    // move the entry
    bucket->item_vec[unused_item_index] = current_extra_bucket->item_vec[moved_item_index];
    current_extra_bucket->item_vec[moved_item_index] = 0;

    // if it was the last entry of current_extra_bucket, free current_extra_bucket
    if (last_item)
        mehcached_free_extra_bucket(table, prev_extra_bucket);
}

size_t
mehcached_find_empty(struct mehcached_table *table, struct mehcached_bucket *bucket, struct mehcached_bucket **located_bucket)
{
    struct mehcached_bucket *current_bucket = bucket;
    while (true)
    {
        size_t item_index;
        for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        {
            if (current_bucket->item_vec[item_index] == 0)
            {
                *located_bucket = current_bucket;
                return item_index;
            }
        }
        if (!mehcached_has_extra_bucket(current_bucket))
            break;
        current_bucket = mehcached_extra_bucket(table, current_bucket->next_extra_bucket_index);
    }

    // no space; alloc new extra_bucket
    if (mehcached_alloc_extra_bucket(table, current_bucket))
    {
        *located_bucket = mehcached_extra_bucket(table, current_bucket->next_extra_bucket_index);
        return 0;   // use the first slot (it should be empty)
    }
    else
    {
        // no free extra_bucket
        *located_bucket = NULL;
        return MEHCACHED_ITEMS_PER_BUCKET;
    }
}

size_t
mehcached_find_empty_or_oldest(const struct mehcached_table *table, struct mehcached_bucket *bucket, struct mehcached_bucket **located_bucket)
{
#if defined(MEHCACHED_ALLOC_MALLOC) || defined(MEHCACHED_ALLOC_DYNAMIC)
    // this code should be reachable because we can now use MEHCACHED_NO_EVICTION
    assert(false);
#endif

#ifdef MEHCACHED_ALLOC_POOL
    size_t oldest_item_index = 0;
    uint64_t oldest_item_distance = (uint64_t)-1;
    struct mehcached_bucket *oldest_item_bucket = NULL;
#endif

    struct mehcached_bucket *current_bucket = bucket;
    while (true)
    {
        size_t item_index;
        for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        {
            if (current_bucket->item_vec[item_index] == 0)
            {
                *located_bucket = current_bucket;
                return item_index;
            }
#ifdef MEHCACHED_ALLOC_POOL
            uint8_t alloc_id = MEHCACHED_ALLOC_ID(current_bucket->item_vec[item_index]);
            uint64_t distance = (table->alloc[alloc_id].tail - MEHCACHED_ITEM_OFFSET(current_bucket->item_vec[item_index])) & MEHCACHED_ITEM_OFFSET_MASK;
            if (oldest_item_distance > distance)
            {
                oldest_item_distance = distance;
                oldest_item_index = item_index;
                oldest_item_bucket = current_bucket;
            }
#endif
        }
        if (!mehcached_has_extra_bucket(current_bucket))
            break;
        current_bucket = mehcached_extra_bucket(table, current_bucket->next_extra_bucket_index);
    }

#ifdef MEHCACHED_ALLOC_POOL
    *located_bucket = oldest_item_bucket;
    return oldest_item_index;
#endif
#if defined(MEHCACHED_ALLOC_MALLOC) || defined(MEHCACHED_ALLOC_DYNAMIC)
    // XXX: getting oldest item is unclear with malloc; just pick anything
    *located_bucket = current_bucket;
    static size_t v = 0;    // XXX: this is slow with multiple threads!
    size_t item_index = (v++) % MEHCACHED_ITEMS_PER_BUCKET;
    return item_index;
#endif
}

void
mehcached_set_item(struct mehcached_item *item, uint64_t key_hash, const uint8_t *key, uint32_t key_length, const uint8_t *value, uint32_t value_length, uint32_t expire_time)
{
    assert(key_length <= MEHCACHED_KEY_MASK);
    assert(value_length <= MEHCACHED_VALUE_MASK);

    item->kv_length_vec = MEHCACHED_KV_LENGTH_VEC(key_length, value_length);
    item->key_hash = key_hash;
    item->expire_time = expire_time;
    mehcached_memcpy8(item->data, key, key_length);
    mehcached_memcpy8(item->data + MEHCACHED_ROUNDUP8(key_length), value, value_length);
}

void
mehcached_set_item_value(struct mehcached_item *item, const uint8_t *value, uint32_t value_length, uint32_t expire_time)
{
    assert(value_length <= MEHCACHED_VALUE_MASK);

    uint32_t key_length = MEHCACHED_KEY_LENGTH(item->kv_length_vec);
    item->kv_length_vec = MEHCACHED_KV_LENGTH_VEC(key_length, value_length);
    item->expire_time = expire_time;
    mehcached_memcpy8(item->data + MEHCACHED_ROUNDUP8(key_length), value, value_length);
}

bool
mehcached_compare_keys(const uint8_t *key1, size_t key1_len, const uint8_t *key2, size_t key2_len)
{
    return key1_len == key2_len && mehcached_memcmp8(key1, key2, key1_len);
}

size_t
mehcached_find_item_index(const struct mehcached_table *table, struct mehcached_bucket *bucket, uint64_t key_hash, uint16_t tag, const uint8_t *key, size_t key_length, struct mehcached_bucket **located_bucket)
{
    struct mehcached_bucket *current_bucket = bucket;

    while (true)
    {
        size_t item_index;
        for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        {
            if (MEHCACHED_TAG(current_bucket->item_vec[item_index]) != tag)
                continue;

            // we may read garbage values, which do not cause any fatal issue
    #ifdef MEHCACHED_ALLOC_POOL
            uint8_t alloc_id = MEHCACHED_ALLOC_ID(current_bucket->item_vec[item_index]);
            struct mehcached_item *item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[alloc_id], MEHCACHED_ITEM_OFFSET(current_bucket->item_vec[item_index]));
    #endif
    #ifdef MEHCACHED_ALLOC_MALLOC
            struct mehcached_item *item = (struct mehcached_item *)mehcached_malloc_item(&table->alloc, MEHCACHED_ITEM_OFFSET(current_bucket->item_vec[item_index]));
    #endif
    #ifdef MEHCACHED_ALLOC_DYNAMIC
            struct mehcached_item *item = (struct mehcached_item *)mehcached_dynamic_item(&table->alloc, MEHCACHED_ITEM_OFFSET(current_bucket->item_vec[item_index]));
    #endif

            if (item->key_hash != key_hash)
                continue;

            // a key comparison reads up to min(source key length and destination key length), which is always safe to do
            if (!mehcached_compare_keys(item->data, MEHCACHED_KEY_LENGTH(item->kv_length_vec), key, key_length))
                continue;

            // we skip any validity check because it will be done by callers who are doing more jobs with this result

    #ifdef MEHCACHED_VERBOSE
            printf("find item index: %zu\n", item_index);
    #endif
            *located_bucket = current_bucket;
            return item_index;
        }

        if (!mehcached_has_extra_bucket(current_bucket))
            break;
        current_bucket = mehcached_extra_bucket(table, current_bucket->next_extra_bucket_index);
    }

#ifdef MEHCACHED_VERBOSE
    printf("could not find item index\n");
#endif

    *located_bucket = NULL;
    return MEHCACHED_ITEMS_PER_BUCKET;
}

size_t
mehcached_find_same_tag(const struct mehcached_table *table, struct mehcached_bucket *bucket, uint16_t tag, struct mehcached_bucket **located_bucket)
{
    struct mehcached_bucket *current_bucket = bucket;

    while (true)
    {
        size_t item_index;
        for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
        {
            if (MEHCACHED_TAG(current_bucket->item_vec[item_index]) != tag)
                continue;

            *located_bucket = current_bucket;
            return item_index;
        }

        if (!mehcached_has_extra_bucket(current_bucket))
            break;
        current_bucket = mehcached_extra_bucket(table, current_bucket->next_extra_bucket_index);
    }

    *located_bucket = NULL;
    return MEHCACHED_ITEMS_PER_BUCKET;
}

#ifdef MEHCACHED_ALLOC_POOL
void
mehcached_cleanup_bucket(uint8_t current_alloc_id, struct mehcached_table *table, uint64_t old_tail, uint64_t new_tail)
{
    // XXX: in principal, we should use old/new head for cleanup instead of tails because head changes are what invalidates locations.
    // however, using tails does the same job as using heads by large because tail change >= head change.

    // tails are using 64-bit numbers, but it is OK because we apply table->num_buckets_mask mask
    uint64_t bucket_index = (old_tail >> table->rshift) & table->num_buckets_mask;
    uint64_t bucket_index_end = (new_tail >> table->rshift) & table->num_buckets_mask;

    while (bucket_index != bucket_index_end)
    {
#ifdef MEHCACHED_VERBOSE
        printf("cleanup bucket: %lu\n", bucket_index);
#endif
        struct mehcached_bucket *bucket = table->buckets + bucket_index;

        mehcached_lock_bucket(table, bucket);

        struct mehcached_bucket *current_bucket = bucket;
        while (true)
        {
            size_t item_index;
            for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
            {
                uint64_t *item_vec_p = &current_bucket->item_vec[item_index];
                if (*item_vec_p == 0)
                    continue;

                // skip other alloc's items because it will incur access to that allocator
                if (MEHCACHED_ALLOC_ID(*item_vec_p) != current_alloc_id)
                    continue;

                if (!mehcached_pool_is_valid(&table->alloc[current_alloc_id], MEHCACHED_ITEM_OFFSET(*item_vec_p)))
                {
                    *item_vec_p = 0;
                    MEHCACHED_STAT_INC(table, cleanup);
                    MEHCACHED_STAT_DEC(table, count);
                }
            }

            if (!mehcached_has_extra_bucket(current_bucket))
                break;
            current_bucket = mehcached_extra_bucket(table, current_bucket->next_extra_bucket_index);
        }

        mehcached_unlock_bucket(table, bucket);
        bucket_index = (bucket_index + 1UL) & table->num_buckets_mask;
    }
}

void
mehcached_cleanup_all(uint8_t current_alloc_id, struct mehcached_table *table)
{
    mehcached_cleanup_bucket(current_alloc_id, table, 0, (uint64_t)1 << table->rshift);
    mehcached_cleanup_bucket(current_alloc_id, table, (uint64_t)1 << table->rshift, 0);
}
#endif

void
mehcached_prefetch_table(struct mehcached_table *table, uint64_t key_hash, struct mehcached_prefetch_state *out_prefetch_state)
{
    struct mehcached_prefetch_state *prefetch_state = out_prefetch_state;

    uint32_t bucket_index = mehcached_calc_bucket_index(table, key_hash);

    prefetch_state->table = table;
    prefetch_state->bucket = table->buckets + bucket_index;
    prefetch_state->key_hash = key_hash;

    // bucket address is already 64-byte aligned
    __builtin_prefetch(prefetch_state->bucket, 0, 0);

    if (MEHCACHED_ITEMS_PER_BUCKET > 7)
        __builtin_prefetch((uint8_t *)prefetch_state->bucket + 64, 0, 0);

    // XXX: prefetch extra buckets, too?
}

void
mehcached_prefetch_alloc(struct mehcached_prefetch_state *in_out_prefetch_state)
{
    struct mehcached_prefetch_state *prefetch_state = in_out_prefetch_state;

    struct mehcached_table *table = prefetch_state->table;
    struct mehcached_bucket *bucket = prefetch_state->bucket;
    uint16_t tag = mehcached_calc_tag(prefetch_state->key_hash);

    size_t item_index;
    for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
    {
        if (MEHCACHED_TAG(bucket->item_vec[item_index]) != tag)
            continue;

#ifdef MEHCACHED_ALLOC_POOL
        uint8_t alloc_id = MEHCACHED_ALLOC_ID(bucket->item_vec[item_index]);
        struct mehcached_item *item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[alloc_id], MEHCACHED_ITEM_OFFSET(bucket->item_vec[item_index]));
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
        struct mehcached_item *item = (struct mehcached_item *)mehcached_malloc_item(&table->alloc, MEHCACHED_ITEM_OFFSET(bucket->item_vec[item_index]));
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
        struct mehcached_item *item = (struct mehcached_item *)mehcached_dynamic_item(&table->alloc, MEHCACHED_ITEM_OFFSET(bucket->item_vec[item_index]));
#endif

        // prefetch the item's cache line and the subsequence cache line
        __builtin_prefetch((void *)(((size_t)item & ~(size_t)63) + 0), 0, 0);
        __builtin_prefetch((void *)(((size_t)item & ~(size_t)63) + 64), 0, 0);
    }
}

bool
mehcached_get(uint8_t current_alloc_id MEHCACHED_UNUSED, struct mehcached_table *table, uint64_t key_hash, const uint8_t *key, size_t key_length, uint8_t *out_value, size_t *in_out_value_length, uint32_t *out_expire_time, bool readonly MEHCACHED_UNUSED)
{
    assert(key_length <= MEHCACHED_MAX_KEY_LENGTH);

    uint32_t bucket_index = mehcached_calc_bucket_index(table, key_hash);
    uint16_t tag = mehcached_calc_tag(key_hash);

    struct mehcached_bucket *bucket = table->buckets + bucket_index;

    bool partial_value;
    uint32_t expire_time;

    while (true)
    {
        uint32_t version_start = mehcached_read_version_begin(table, bucket);

        struct mehcached_bucket *located_bucket;
        size_t item_index = mehcached_find_item_index(table, bucket, key_hash, tag, key, key_length, &located_bucket);
        if (item_index == MEHCACHED_ITEMS_PER_BUCKET)
        {
            if (version_start != mehcached_read_version_end(table, bucket))
                continue;
            MEHCACHED_STAT_INC(table, get_notfound);
            return false;
        }

        uint64_t item_vec = located_bucket->item_vec[item_index];
        uint64_t item_offset = MEHCACHED_ITEM_OFFSET(item_vec);

#ifdef MEHCACHED_ALLOC_POOL
        // we may read garbage data, but all operations relying on them are safe here
        uint64_t alloc_id = MEHCACHED_ALLOC_ID(item_vec);
        if (alloc_id >= (uint64_t)table->alloc_id_mask + 1)   // fix-up for possible garbage read
            alloc_id = 0;
        struct mehcached_item *item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[alloc_id], item_offset);
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
        struct mehcached_item *item = (struct mehcached_item *)mehcached_malloc_item(&table->alloc, item_offset);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
        struct mehcached_item *item = (struct mehcached_item *)mehcached_dynamic_item(&table->alloc, item_offset);
#endif

        expire_time = item->expire_time;

        size_t key_length = MEHCACHED_KEY_LENGTH(item->kv_length_vec);
        if (key_length > MEHCACHED_MAX_KEY_LENGTH)
            key_length = MEHCACHED_MAX_KEY_LENGTH;  // fix-up for possible garbage read

        size_t value_length = MEHCACHED_VALUE_LENGTH(item->kv_length_vec);
        if (value_length > MEHCACHED_MAX_VALUE_LENGTH)
            value_length = MEHCACHED_MAX_VALUE_LENGTH;  // fix-up for possible garbage read

#ifndef NDEBUG
        // debug code to check how the code works when it read garbage values
        // if all inserted values are no larger than 1500 and this debug message is printed, something bad is occurring
        if (value_length > 1500)
        {
#ifdef MEHCACHED_ALLOC_POOL
            fprintf(stderr, "head %lu\n", table->alloc[alloc_id].head);
            fprintf(stderr, "tail %lu\n", table->alloc[alloc_id].tail);
#endif
            fprintf(stderr, "item_offset %lu\n", item_offset);
        }
#endif

#ifdef MEHCACHED_ALLOC_POOL
        // for move-to-head
        // this does not use item->alloc_item.item_size to discard currently unused space within the item
        uint32_t item_size = (uint32_t)(sizeof(struct mehcached_item) + MEHCACHED_ROUNDUP8(key_length) + MEHCACHED_ROUNDUP8(value_length));
#endif

        // adjust value length to use
        if (value_length > *in_out_value_length)
        {
            partial_value = true;
            value_length = *in_out_value_length;
        }
        else
            partial_value = false;
        mehcached_memcpy8(out_value, item->data + MEHCACHED_ROUNDUP8(key_length), value_length);

#ifdef MEHCACHED_ALLOC_POOL
        if (!mehcached_pool_is_valid(&table->alloc[alloc_id], item_offset))
        {
            if (version_start != mehcached_read_version_end(table, bucket))
                continue;

            if (!readonly)
            {
                // remove stale item; this may remove some wrong item, but we do not care because
                // (1) if this key has been deleted and reinserted, it is none of our business here
                // (2) if this key has been deleted and a different key was inserted, we delete innocent key, but without any fatal issue.
                // this will slow down query speed for outdated matching key at first, but improves it later by skipping the value copy step
                mehcached_lock_bucket(table, bucket);
                if (located_bucket->item_vec[item_index] != 0)
                {
                    located_bucket->item_vec[item_index] = 0;
                    MEHCACHED_STAT_DEC(table, count);
                }
                mehcached_unlock_bucket(table, bucket);
            }

            MEHCACHED_STAT_INC(table, get_notfound);
            return false;
        }
#endif

        if (version_start != mehcached_read_version_end(table, bucket))
            continue;

#ifndef NDEBUG
        // debug code to check how the code works when it read garbage values
        if (value_length > 1500)
            assert(false);
#endif

        *in_out_value_length = value_length;
        if (out_expire_time != NULL)
            *out_expire_time = expire_time;

        // the following is optional processing (we will return the value retrieved above)

        MEHCACHED_STAT_INC(table, get_found);

#ifdef MEHCACHED_ALLOC_POOL
        if (!readonly && alloc_id == current_alloc_id)
        {
            // small distance_from_tail = recently appended at tail
            // it is not required to acquire a lock to read table->alloc.tail because
            // some inaccurate number is OK
            // note that we are accessing table->alloc.tail, which uses a 64-bit number; this is OK because we apply table->alloc.mask mask
            // perform move-to-head within the same alloc only

            uint64_t distance_from_tail = (table->alloc[current_alloc_id].tail - item_offset) & table->alloc[current_alloc_id].mask;
            if (distance_from_tail > table->mth_threshold)
            {
                mehcached_lock_bucket(table, bucket);
                mehcached_pool_lock(&table->alloc[current_alloc_id]);

                // check if the original item is still there
                if (located_bucket->item_vec[item_index] == item_vec)
                {
                    // allocate new space
                    uint64_t new_item_offset = mehcached_pool_allocate(&table->alloc[current_alloc_id], item_size);
                    uint64_t new_tail = table->alloc[current_alloc_id].tail;

                    // see if the original item is still valid because mehcached_pool_allocate() by this thread or other threads may have invalidated it
                    if (mehcached_pool_is_valid(&table->alloc[current_alloc_id], item_offset))
                    {
                        struct mehcached_item *new_item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[current_alloc_id], new_item_offset);
                        mehcached_memcpy8((uint8_t *)new_item, (const uint8_t *)item, item_size);

                        uint64_t new_item_vec = MEHCACHED_ITEM_VEC(MEHCACHED_TAG(item_vec), current_alloc_id, new_item_offset);

                        located_bucket->item_vec[item_index] = new_item_vec;

                        // success
                        MEHCACHED_STAT_INC(table, move_to_head_performed);
                    }
                    else
                    {
                        // failed -- original data become invalid in the alloc
                        MEHCACHED_STAT_INC(table, move_to_head_failed);
                    }

                    // we need to hold the lock until we finish writing
                    mehcached_pool_unlock(&table->alloc[current_alloc_id]);
                    mehcached_unlock_bucket(table, bucket);

                    // XXX: this may be too late; before cleaning, other threads may have read some invalid location
                    //      ideally, this should be done before writing actual data
                    mehcached_cleanup_bucket(current_alloc_id, table, new_item_offset, new_tail);
                }
                else
                {
                    mehcached_pool_unlock(&table->alloc[current_alloc_id]);
                    mehcached_unlock_bucket(table, bucket);

                    // failed -- original data become invalid in the table
                    MEHCACHED_STAT_INC(table, move_to_head_failed);
                }
            }
            else
            {
                MEHCACHED_STAT_INC(table, move_to_head_skipped);
            }
        }
#endif
        break;
    }

    // TODO: check partial_value and return a correct code
    (void)partial_value;

#ifndef NDEBUG
    // debug code to check how the code works when it read garbage values
    if (*in_out_value_length > 1500)
        assert(false);
#endif

    return true;
}

bool
mehcached_test(uint8_t current_alloc_id MEHCACHED_UNUSED, struct mehcached_table *table, uint64_t key_hash, const uint8_t *key, size_t key_length)
{
    assert(key_length <= MEHCACHED_MAX_KEY_LENGTH);

    uint32_t bucket_index = mehcached_calc_bucket_index(table, key_hash);
    uint16_t tag = mehcached_calc_tag(key_hash);

    struct mehcached_bucket *bucket = table->buckets + bucket_index;

    while (true)
    {
        uint32_t version_start = mehcached_read_version_begin(table, bucket);

        struct mehcached_bucket *located_bucket;
        size_t item_index = mehcached_find_item_index(table, bucket, key_hash, tag, key, key_length, &located_bucket);

        if (version_start != mehcached_read_version_end(table, bucket))
            continue;

        if (item_index != MEHCACHED_ITEMS_PER_BUCKET)
        {
            MEHCACHED_STAT_INC(table, test_found);
            return true;
        }
        else
        {
            MEHCACHED_STAT_INC(table, test_notfound);
            return false;
        }
    }

    // not reachable
    return false;
}

bool
mehcached_set(uint8_t current_alloc_id, struct mehcached_table *table, uint64_t key_hash, const uint8_t *key, size_t key_length, const uint8_t *value, size_t value_length, uint32_t expire_time, bool overwrite)
{
    assert(key_length <= MEHCACHED_MAX_KEY_LENGTH);
    assert(value_length <= MEHCACHED_MAX_VALUE_LENGTH);

    uint32_t bucket_index = mehcached_calc_bucket_index(table, key_hash);
    uint16_t tag = mehcached_calc_tag(key_hash);

    struct mehcached_bucket *bucket = table->buckets + bucket_index;

    bool overwriting;

    mehcached_lock_bucket(table, bucket);

    struct mehcached_bucket *located_bucket;
    size_t item_index = mehcached_find_item_index(table, bucket, key_hash, tag, key, key_length, &located_bucket);

    if (item_index != MEHCACHED_ITEMS_PER_BUCKET)
    {
        if (!overwrite)
        {
            MEHCACHED_STAT_INC(table, set_nooverwrite);

            mehcached_unlock_bucket(table, bucket);
            return false;   // already exist but cannot overwrite
        }
        else
        {
            overwriting = true;
        }
    }
    else
    {
#ifndef MEHCACHED_NO_EVICTION
        // pick an item with the same tag; this is potentially the same item but invalid due to insufficient log space
        // this helps limiting the effect of "ghost" items in the table when the log space is slightly smaller than enough
        // and makes better use of the log space by not evicting unrelated old items in the same bucket
        item_index = mehcached_find_same_tag(table, bucket, tag, &located_bucket);

        if (item_index == MEHCACHED_ITEMS_PER_BUCKET)
        {
            // if there is no matching tag, we just use the empty or oldest item
            item_index = mehcached_find_empty_or_oldest(table, bucket, &located_bucket);
        }
#else
        item_index = mehcached_find_empty(table, bucket, &located_bucket);
        if (item_index == MEHCACHED_ITEMS_PER_BUCKET)
        {
            // no more space
            // TODO: add a statistics entry
            mehcached_unlock_bucket(table, bucket);
            return false;
        }
#endif

        overwriting = false;
    }

    uint32_t new_item_size = (uint32_t)(sizeof(struct mehcached_item) + MEHCACHED_ROUNDUP8(key_length) + MEHCACHED_ROUNDUP8(value_length));
    uint64_t item_offset;

#ifdef MEHCACHED_ALLOC_POOL
    // we have to lock the pool because is_valid check must be correct in the overwrite mode;
    // unlike reading, we cannot afford writing data at a wrong place
    mehcached_pool_lock(&table->alloc[current_alloc_id]);
#endif

    if (overwriting)
    {
        item_offset = MEHCACHED_ITEM_OFFSET(located_bucket->item_vec[item_index]);

        if (MEHCACHED_ALLOC_ID(located_bucket->item_vec[item_index]) == current_alloc_id)
        {
            // do in-place update only when alloc_id matches to avoid write threshing

#ifdef MEHCACHED_ALLOC_POOL
            struct mehcached_item *item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[current_alloc_id], item_offset);
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
            struct mehcached_item *item = (struct mehcached_item *)mehcached_malloc_item(&table->alloc, item_offset);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
            struct mehcached_item *item = (struct mehcached_item *)mehcached_dynamic_item(&table->alloc, item_offset);
#endif

#ifdef MEHCACHED_ALLOC_POOL
            if (mehcached_pool_is_valid(&table->alloc[current_alloc_id], item_offset))
#endif
            {
                if (item->alloc_item.item_size >= new_item_size)
                {
                    MEHCACHED_STAT_INC(table, set_inplace);

                    mehcached_set_item_value(item, value, (uint32_t)value_length, expire_time);

#ifdef MEHCACHED_ALLOC_POOL
                    mehcached_pool_unlock(&table->alloc[current_alloc_id]);
#endif
                    mehcached_unlock_bucket(table, bucket);
                    return true;
                }
            }
        }
    }

#ifdef MEHCACHED_ALLOC_POOL
    uint64_t new_item_offset = mehcached_pool_allocate(&table->alloc[current_alloc_id], new_item_size);
    uint64_t new_tail = table->alloc[current_alloc_id].tail;
    struct mehcached_item *new_item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[current_alloc_id], new_item_offset);
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
    uint64_t new_item_offset = mehcached_malloc_allocate(&table->alloc, new_item_size);
    if (new_item_offset == MEHCACHED_MALLOC_INSUFFICIENT_SPACE)
    {
        // no more space
        // TODO: add a statistics entry
        mehcached_unlock_bucket(table, bucket);
        return false;
    }
    struct mehcached_item *new_item = (struct mehcached_item *)mehcached_malloc_item(&table->alloc, new_item_offset);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
    mehcached_dynamic_lock(&table->alloc);
    uint64_t new_item_offset = mehcached_dynamic_allocate(&table->alloc, new_item_size);
    mehcached_dynamic_unlock(&table->alloc);
    if (new_item_offset == MEHCACHED_DYNAMIC_INSUFFICIENT_SPACE)
    {
        // no more space
        // TODO: add a statistics entry
        mehcached_unlock_bucket(table, bucket);
        return false;
    }
    struct mehcached_item *new_item = (struct mehcached_item *)mehcached_dynamic_item(&table->alloc, new_item_offset);
#endif

    MEHCACHED_STAT_INC(table, set_new);

    mehcached_set_item(new_item, key_hash, key, (uint32_t)key_length, value, (uint32_t)value_length, expire_time);
#ifdef MEHCACHED_ALLOC_POOL
    // unlocking is delayed until we finish writing data at the new location;
    // otherwise, the new location may be invalidated (in a very rare case)
    mehcached_pool_unlock(&table->alloc[current_alloc_id]);
#endif

    if (located_bucket->item_vec[item_index] != 0)
    {
        MEHCACHED_STAT_INC(table, set_evicted);
        MEHCACHED_STAT_DEC(table, count);
    }

    located_bucket->item_vec[item_index] = MEHCACHED_ITEM_VEC(tag, current_alloc_id, new_item_offset);

    mehcached_unlock_bucket(table, bucket);

#ifdef MEHCACHED_ALLOC_POOL
    // XXX: this may be too late; before cleaning, other threads may have read some invalid location
    //      ideally, this should be done before writing actual data
    mehcached_cleanup_bucket(current_alloc_id, table, new_item_offset, new_tail);
#endif

#ifdef MEHCACHED_ALLOC_MALLOC
    // this is done after bucket is updated and unlocked
    if (overwriting)
        mehcached_malloc_deallocate(&table->alloc, item_offset);
#endif

#ifdef MEHCACHED_ALLOC_DYNAMIC
    // this is done after bucket is updated and unlocked
    if (overwriting)
    {
        mehcached_dynamic_lock(&table->alloc);
        mehcached_dynamic_deallocate(&table->alloc, item_offset);
        mehcached_dynamic_unlock(&table->alloc);
    }
#endif

    MEHCACHED_STAT_INC(table, count);

    return true;
}

bool
mehcached_delete(uint8_t alloc_id MEHCACHED_UNUSED, struct mehcached_table *table, uint64_t key_hash, const uint8_t *key, size_t key_length)
{
    assert(key_length <= MEHCACHED_MAX_KEY_LENGTH);

    uint32_t bucket_index = mehcached_calc_bucket_index(table, key_hash);
    uint16_t tag = mehcached_calc_tag(key_hash);

    struct mehcached_bucket *bucket = table->buckets + bucket_index;

    mehcached_lock_bucket(table, bucket);

    struct mehcached_bucket *located_bucket;
    size_t item_index = mehcached_find_item_index(table, bucket, key_hash, tag, key, key_length, &located_bucket);
    if (item_index == MEHCACHED_ITEMS_PER_BUCKET)
    {
        mehcached_unlock_bucket(table, bucket);
        MEHCACHED_STAT_INC(table, delete_notfound);
        return false;
    }

    located_bucket->item_vec[item_index] = 0;
    MEHCACHED_STAT_DEC(table, count);

    mehcached_fill_hole(table, located_bucket, item_index);

    mehcached_unlock_bucket(table, bucket);

    MEHCACHED_STAT_INC(table, delete_found);
    return true;
}

bool
mehcached_increment(uint8_t current_alloc_id, struct mehcached_table *table, uint64_t key_hash, const uint8_t *key, size_t key_length, uint64_t increment, uint64_t *out_new_value, uint32_t expire_time)
{
    assert(key_length <= MEHCACHED_MAX_KEY_LENGTH);

    uint32_t bucket_index = mehcached_calc_bucket_index(table, key_hash);
    uint16_t tag = mehcached_calc_tag(key_hash);

    struct mehcached_bucket *bucket = table->buckets + bucket_index;

    // TODO: add stats

    mehcached_lock_bucket(table, bucket);

    struct mehcached_bucket *located_bucket;
    size_t item_index = mehcached_find_item_index(table, bucket, key_hash, tag, key, key_length, &located_bucket);

    if (item_index == MEHCACHED_ITEMS_PER_BUCKET)
    {
        mehcached_unlock_bucket(table, bucket);
        // TODO: support seeding a new item with the given default value?
        return false;   // does not exist
    }

    if (current_alloc_id != MEHCACHED_ALLOC_ID(located_bucket->item_vec[item_index]))
    {
        mehcached_unlock_bucket(table, bucket);
        return false;   // writing to a different alloc is not allowed
    }

    uint64_t item_offset = MEHCACHED_ITEM_OFFSET(located_bucket->item_vec[item_index]);

#ifdef MEHCACHED_ALLOC_POOL
    // ensure that the item is still valid
    mehcached_pool_lock(&table->alloc[current_alloc_id]);

    if (!mehcached_pool_is_valid(&table->alloc[current_alloc_id], item_offset))
    {
        mehcached_pool_unlock(&table->alloc[current_alloc_id]);
        mehcached_unlock_bucket(table, bucket);
        // TODO: support seeding a new item with the given default value?
        return false;   // exists in the table but not valid in the pool
    }
#endif

#ifdef MEHCACHED_ALLOC_POOL
    struct mehcached_item *item = (struct mehcached_item *)mehcached_pool_item(&table->alloc[current_alloc_id], item_offset);
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
    struct mehcached_item *item = (struct mehcached_item *)mehcached_malloc_item(&table->alloc, item_offset);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
    struct mehcached_item *item = (struct mehcached_item *)mehcached_dynamic_item(&table->alloc, item_offset);
#endif

    size_t value_length = MEHCACHED_VALUE_LENGTH(item->kv_length_vec);
    if (value_length != sizeof(uint64_t))
    {
#ifdef MEHCACHED_ALLOC_POOL
        mehcached_pool_unlock(&table->alloc[current_alloc_id]);
#endif
        mehcached_unlock_bucket(table, bucket);
        return false;   // invalid value size
    }

    uint64_t old_value;
    mehcached_memcpy8((uint8_t *)&old_value, item->data + MEHCACHED_ROUNDUP8(key_length), sizeof(old_value));

    uint64_t new_value = old_value + increment;
    mehcached_memcpy8(item->data + MEHCACHED_ROUNDUP8(key_length), (const uint8_t *)&new_value, sizeof(new_value));
    item->expire_time = expire_time;

    *out_new_value = new_value;

#ifdef MEHCACHED_ALLOC_POOL
    mehcached_pool_unlock(&table->alloc[current_alloc_id]);
#endif
    mehcached_unlock_bucket(table, bucket);

    return true;
}

/*
static
void
mehcached_process_batch(uint8_t current_alloc_id, struct mehcached_table *table, struct mehcached_request *requests, size_t num_requests, const uint8_t *in_data, uint8_t *out_data, size_t *out_data_length, bool readonly)
{
    assert(sizeof(struct mehcached_request) == 24);

    const uint8_t *in_data_p = in_data;

    uint8_t *out_data_p = out_data;
    const uint8_t *out_data_end = out_data + *out_data_length;

#ifdef MEHCACHED_DEDUP_WITHIN_BATCH
    bool dup[num_requests];
    size_t key_lengths[num_requests];
    size_t value_lengths[num_requests];
    const uint8_t *keys[num_requests];
    memset(dup, 0, sizeof(dup));
    {
        size_t request_index;
        size_t request_index_t;

        for (request_index = 0; request_index < num_requests; request_index++)
        {
            struct mehcached_request *req = (struct mehcached_request *)requests + request_index;
            size_t key_length = MEHCACHED_KEY_LENGTH(req->kv_length_vec);
            size_t value_length = MEHCACHED_VALUE_LENGTH(req->kv_length_vec);
            const uint8_t *key = in_data_p;
            key_lengths[request_index] = key_length;
            value_lengths[request_index] = key_length;
            keys[request_index] = key;
            in_data_p += MEHCACHED_ROUNDUP8(key_length) + MEHCACHED_ROUNDUP8(value_length);
        }

        for (request_index = 0; request_index < num_requests - 1; request_index++)
        {
            struct mehcached_request *req = (struct mehcached_request *)requests + request_index;
            if (req->operation != MEHCACHED_SET)
                continue;
            for (request_index_t = request_index + 1; request_index_t < num_requests; request_index_t++)
            {
                struct mehcached_request *req_t = (struct mehcached_request *)requests + request_index_t;
                if (req_t->operation == MEHCACHED_SET)
                {
                    if (req->key_hash == req_t->key_hash && mehcached_compare_keys(keys[request_index], key_lengths[request_index], keys[request_index_t], key_lengths[request_index_t]))
                    {
                        // this request can be skipped
                        dup[request_index] = true;
                        break;
                    }
                }
                else if (req_t->operation != MEHCACHED_SET)
                {
                    if (req->key_hash == req_t->key_hash && mehcached_compare_keys(keys[request_index], key_lengths[request_index], keys[request_index_t], key_lengths[request_index_t]))
                    {
                        // this request must be ran for the later operation
                        break;
                    }
                }
            }
        }
    }

#endif

    // TODO: prefetching?
    size_t request_index;
    for (request_index = 0; request_index < num_requests; request_index++)
    {
        struct mehcached_request *req = (struct mehcached_request *)requests + request_index;
#ifndef MEHCACHED_DEDUP_WITHIN_BATCH
        size_t key_length = MEHCACHED_KEY_LENGTH(req->kv_length_vec);
        size_t value_length = MEHCACHED_VALUE_LENGTH(req->kv_length_vec);
        const uint8_t *key = in_data_p;
#else
        size_t key_length = key_lengths[request_index];
        size_t value_length = value_lengths[request_index];
        const uint8_t *key = keys[request_index];
#endif
        const uint8_t *value = key + MEHCACHED_ROUNDUP8(key_length);
        in_data_p += MEHCACHED_ROUNDUP8(key_length) + MEHCACHED_ROUNDUP8(value_length);

        switch (req->operation)
        {
            case MEHCACHED_NOOP_READ:
            case MEHCACHED_NOOP_WRITE:
                {
                    req->result = MEHCACHED_OK;
                    req->kv_length_vec = 0;
                }
                break;
            case MEHCACHED_ADD:
            case MEHCACHED_SET:
                {
                    // TODO: dedup should handle a mixture or ADD and SET correctly
                    bool overwrite = req->operation == MEHCACHED_SET;
#ifdef MEHCACHED_DEDUP_WITHIN_BATCH
                    if (dup[request_index])
                        req->result = MEHCACHED_OK;
                    else
                    {
#endif
                    if (mehcached_set(current_alloc_id, table, req->key_hash, key, key_length, value, value_length, req->expire_time, overwrite))
                        req->result = MEHCACHED_OK;
                    else
                        req->result = MEHCACHED_ERROR;  // TODO: return a correct failure code
#ifdef MEHCACHED_DEDUP_WITHIN_BATCH
                    }
#endif
                    req->kv_length_vec = 0;
                    break;
                }
                break;
            case MEHCACHED_GET:
                {
                    size_t out_value_length = (size_t)(out_data_end - out_data_p);
                    uint8_t *out_value = out_data_p;
                    uint32_t expire_time = req->expire_time;
                    if (mehcached_get(current_alloc_id, table, req->key_hash, key, key_length, out_value, &out_value_length, &expire_time, readonly))
                        req->result = MEHCACHED_OK;
                    else
                    {
                        req->result = MEHCACHED_ERROR;  // TODO: return a correct failure code
                        out_value_length = 0;
                    }
                    req->kv_length_vec = MEHCACHED_KV_LENGTH_VEC(0, out_value_length);
                    req->expire_time = expire_time;
                    out_data_p += MEHCACHED_ROUNDUP8(out_value_length);
                    break;
                }
                break;
            case MEHCACHED_TEST:
                {
                    if (mehcached_test(current_alloc_id, table, req->key_hash, key, key_length))
                        req->result = MEHCACHED_OK;
                    else
                        req->result = MEHCACHED_ERROR;  // TODO: return a correct failure code
                    req->kv_length_vec = 0;
                    break;
                }
                break;
            case MEHCACHED_DELETE:
                {
                    if (mehcached_delete(current_alloc_id, table, req->key_hash, key, key_length))
                        req->result = MEHCACHED_OK;
                    else
                        req->result = MEHCACHED_ERROR;  // TODO: return a correct failure code
                    req->kv_length_vec = 0;
                    break;
                }
                break;
            case MEHCACHED_INCREMENT:
                {
                    // TODO: check if the output space is large enough
                    uint64_t increment;
                    assert(value_length == sizeof(uint64_t));
                    mehcached_memcpy8((uint8_t *)&increment, value, sizeof(uint64_t));
                    size_t out_value_length = sizeof(uint64_t);
                    uint8_t *out_value = out_data_p;
                    if (mehcached_increment(current_alloc_id, table, req->key_hash, key, key_length, increment, (uint64_t *)out_value, req->expire_time))
                        req->result = MEHCACHED_OK;
                    else
                    {
                        req->result = MEHCACHED_ERROR;  // TODO: return a correct failure code
                        out_value_length = 0;
                    }
                    req->kv_length_vec = MEHCACHED_KV_LENGTH_VEC(0, out_value_length);
                    out_data_p += MEHCACHED_ROUNDUP8(out_value_length);
                    break;
                }
                break;
            default:
                fprintf(stderr, "invalid operation\n");
                break;
        }
    }
    *out_data_length = (size_t)(out_data_p - out_data);
}
*/


void
mehcached_table_reset(struct mehcached_table *table)
{
    size_t bucket_index;
#ifdef MEHCACHED_ALLOC_DYNAMIC
    mehcached_dynamic_lock(&table->alloc);
#endif
    for (bucket_index = 0; bucket_index < table->num_buckets; bucket_index++)
    {
        struct mehcached_bucket *bucket = table->buckets + bucket_index;
        size_t item_index;
        for (item_index = 0; item_index < MEHCACHED_ITEMS_PER_BUCKET; item_index++)
            if (bucket->item_vec[item_index] != 0)
            {
#ifdef MEHCACHED_ALLOC_MALLOC
                mehcached_malloc_deallocate(&table->alloc, MEHCACHED_ITEM_OFFSET(bucket->item_vec[item_index]));
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
                mehcached_dynamic_deallocate(&table->alloc, MEHCACHED_ITEM_OFFSET(bucket->item_vec[item_index]));
#endif
            }
    }
#ifdef MEHCACHED_ALLOC_DYNAMIC
    mehcached_dynamic_unlock(&table->alloc);
#endif

    memset(table->buckets, 0, sizeof(struct mehcached_bucket) * (table->num_buckets + table->num_extra_buckets));

    // initialize a free list of extra buckets
    if (table->num_extra_buckets == 0)
        table->extra_bucket_free_list.head = 0;    // no extra bucket at all
    else
    {
        uint32_t extra_bucket_index;
        for (extra_bucket_index = 1; extra_bucket_index < 1 + table->num_extra_buckets - 1; extra_bucket_index++)
            (table->extra_buckets + extra_bucket_index)->next_extra_bucket_index = extra_bucket_index + 1;
        (table->extra_buckets + extra_bucket_index)->next_extra_bucket_index = 0;    // no more free extra bucket

        table->extra_bucket_free_list.head = 1;
    }

#ifdef MEHCACHED_ALLOC_POOL
    size_t alloc_id;
    for (alloc_id = 0; alloc_id < (size_t)(table->alloc_id_mask + 1); alloc_id++)
    {
        mehcached_pool_lock(&table->alloc[alloc_id]);
        mehcached_pool_reset(&table->alloc[alloc_id]);
        mehcached_pool_unlock(&table->alloc[alloc_id]);
    }
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
    mehcached_malloc_reset(&table->alloc);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
    mehcached_dynamic_reset(&table->alloc);
#endif

#ifdef MEHCACHED_COLLECT_STATS
    table->stats.count = 0;
#endif
    mehcached_reset_table_stats(table);
}

void
mehcached_table_init(struct mehcached_table *table, size_t num_buckets, size_t num_pools MEHCACHED_UNUSED, size_t pool_size MEHCACHED_UNUSED, bool concurrent_table_read, bool concurrent_table_write, bool concurrent_alloc_write, size_t table_numa_node, size_t alloc_numa_nodes[], double mth_threshold)
{
    assert((MEHCACHED_ITEMS_PER_BUCKET == 7 && sizeof(struct mehcached_bucket) == 64) || (MEHCACHED_ITEMS_PER_BUCKET == 15 && sizeof(struct mehcached_bucket) == 128) || (MEHCACHED_ITEMS_PER_BUCKET == 31 && sizeof(struct mehcached_bucket) == 256));
    assert(sizeof(struct mehcached_item) == 24);

    assert(num_buckets > 0);
    assert(num_pools > 0);
    assert(num_pools <= MEHCACHED_MAX_POOLS);
#ifdef MEHCACHED_SINGLE_ALLOC
    if (num_pools != 1)
    {
        fprintf(stderr, "the program is compiled with no support for multiple pools\n");
        assert(false);
    }
#endif

    size_t log_num_buckets = 0;
    while (((size_t)1 << log_num_buckets) < num_buckets)
        log_num_buckets++;
    num_buckets = (size_t)1 << log_num_buckets;
    assert(log_num_buckets <= 32);

    table->num_buckets = (uint32_t)num_buckets;
    table->num_buckets_mask = (uint32_t)num_buckets - 1;
#ifndef MEHCACHED_NO_EVICTION
    table->num_extra_buckets = 0;
#else
    table->num_extra_buckets = table->num_buckets / 10;    // 10% of normal buckets
#endif

// #ifdef MEHCACHED_ALLOC_POOL
    {
        size_t shm_size = mehcached_shm_adjust_size(sizeof(struct mehcached_bucket) * (table->num_buckets + table->num_extra_buckets));
        // TODO: extend num_buckets to meet shm_size
        size_t shm_id = mehcached_shm_alloc(shm_size, table_numa_node);
        if (shm_id == (size_t)-1)
        {
            printf("failed to allocate memory\n");
            assert(false);
        }
        while (true)
        {
            table->buckets = (struct mehcached_bucket *) mehcached_shm_find_free_address(shm_size);
            if (table->buckets == NULL)
                assert(false);
            if (mehcached_shm_map(shm_id, table->buckets, 0, shm_size))
                break;
        }
        if (!mehcached_shm_schedule_remove(shm_id))
            assert(false);
    }
// #endif
// #ifdef MEHCACHED_ALLOC_MALLOC
//     {
//         int ret = posix_memalign((void **)&table->buckets, 4096, sizeof(struct mehcached_bucket) * (table->num_buckets + table->num_extra_buckets));
//         if (ret != 0)
//             assert(false);
//     }
// #endif
    table->extra_buckets = table->buckets + table->num_buckets - 1; // subtract by one to compensate 1-base indices
    // the rest extra_bucket information is initialized in mehcached_table_reset()

    // we have to zero out buckets here because mehcached_table_reset() tries to free non-zero entries in the buckets
    memset(table->buckets, 0, sizeof(struct mehcached_bucket) * (table->num_buckets + table->num_extra_buckets));

    if (!concurrent_table_read)
        table->concurrent_access_mode = 0;
    else if (concurrent_table_read && !concurrent_table_write)
        table->concurrent_access_mode = 1;
    else
        table->concurrent_access_mode = 2;

#ifdef MEHCACHED_ALLOC_POOL
    num_pools = mehcached_next_power_of_two(num_pools);
    table->alloc_id_mask = (uint8_t)(num_pools - 1);
    size_t alloc_id;
    for (alloc_id = 0; alloc_id < num_pools; alloc_id++)
        mehcached_pool_init(&table->alloc[alloc_id], pool_size, concurrent_table_read, concurrent_alloc_write, alloc_numa_nodes[alloc_id]);
    table->mth_threshold = (uint64_t)((double)table->alloc[0].size * mth_threshold);

    table->rshift = 0;
    while ((((MEHCACHED_ITEM_OFFSET_MASK + 1) >> 1) >> table->rshift) > table->num_buckets)
        table->rshift++;
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
    mehcached_malloc_init(&table->alloc);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
    // TODO: support multiple dynamic allocs?
    mehcached_dynamic_init(&table->alloc, pool_size, concurrent_table_read, concurrent_alloc_write, alloc_numa_nodes[0]);
#endif

    mehcached_table_reset(table);

#ifdef NDEBUG
    printf("NDEBUG\n");
#else
    printf("!NDEBUG (low performance)\n");
#endif

#ifdef MEHCACHED_VERBOSE
    printf("MEHCACHED_VERBOSE (low performance)\n");
#endif

#ifdef MEHCACHED_COLLECT_STATS
    printf("MEHCACHED_COLLECT_STATS (low performance)\n");
#endif

#ifdef MEHCACHED_CONCURRENT
    printf("MEHCACHED_CONCURRENT (low performance)\n");
#endif

    printf("MEHCACHED_MTH_THRESHOLD=%lf (%s)\n", mth_threshold, mth_threshold == 0. ? "LRU" : (mth_threshold == 1. ? "FIFO" : "approx-LRU"));

#ifdef MEHCACHED_USE_PH
    printf("MEHCACHED_USE_PH\n");
#endif

#ifdef MEHCACHED_NO_EVICTION
    printf("MEHCACHED_NO_EVICTION\n");
#endif

#ifdef MEHCACHED_ALLOC_POOL
    printf("MEHCACHED_ALLOC_POOL\n");
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
    printf("MEHCACHED_ALLOC_MALLOC\n");
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
    printf("MEHCACHED_ALLOC_DYNAMIC\n");
#endif
    printf("num_buckets = %u\n", table->num_buckets);
    printf("num_extra_buckets = %u\n", table->num_extra_buckets);
    printf("pool_size = %zu\n", pool_size);

    printf("\n");
}

void
mehcached_table_free(struct mehcached_table *table)
{
    assert(table);

    mehcached_table_reset(table);

// #ifdef MEHCACHED_ALLOC_POOL
    if (!mehcached_shm_unmap(table->buckets))
        assert(false);
// #endif
// #ifdef MEHCACHED_ALLOC_MALLOC
//     free(table->buckets);
// #endif

#ifdef MEHCACHED_ALLOC_POOL
    size_t alloc_id;
    for (alloc_id = 0; alloc_id < (size_t)(table->alloc_id_mask + 1); alloc_id++)
        mehcached_pool_free(&table->alloc[alloc_id]);
#endif
#ifdef MEHCACHED_ALLOC_MALLOC
    mehcached_malloc_free(&table->alloc);
#endif
#ifdef MEHCACHED_ALLOC_DYNAMIC
    mehcached_dynamic_free(&table->alloc);
#endif
}

MEHCACHED_END


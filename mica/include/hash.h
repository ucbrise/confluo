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

#ifndef HASH_H_
#define HASH_H_

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include "citycrc.h"

/*
static uint32_t crc32_le(uint32_t crc, const uint8_t *data, size_t len)
{
    // SSE 4.2 & 64-bit required
    size_t words = len >> 3;
    size_t tail = len & 7;
    while (words)
    {
        words--;
        crc = (uint32_t)__builtin_ia32_crc32di(crc, *(const uint64_t *)data);
        data += 8;
    }
    while (tail)
    {
        tail--;
        crc = __builtin_ia32_crc32qi(crc, *data);
        data++;
    }
    return crc;
}
*/

extern const uint32_t sbox[];

static uint64_t tab_hash(const uint8_t *key, size_t len)
{
    // a large prime number
    uint32_t h = 4294967291U;
    while (len)
    {
        len--;
        // tabulation hashing -- Carter and Wegman (STOC'77)
        h ^= sbox[*key];
        key++;
    }
    return (uint64_t)h;
}

static uint64_t sbox_hash(uint8_t *key, size_t len)
{
    // a large prime number
    uint32_t h = 4294967291U;
    while (len)
    {
        len--;
        h ^= sbox[*key];
        h *= 3;
        key++;
    }
    return (uint64_t)h;
}

static uint64_t noop_hash(const uint8_t *key, size_t len)
{
    assert(len == sizeof(uint64_t));
    (void)len;
    return *(uint64_t *)key;
}

static uint64_t mul_hash(const uint8_t *key, size_t len)
{
    assert(len == sizeof(uint64_t));
    (void)len;
    // a large prime number
    return *(uint64_t *)key * 18446744073709551557UL;
}

// MD4 truncated to 12 B
#include <openssl/md4.h>
static uint64_t hash_md4(const uint8_t *key, size_t len)
{
    size_t temp_hash[(MD4_DIGEST_LENGTH + sizeof(size_t) - 1) / sizeof(size_t)];
    MD4(key, len, (uint8_t *)temp_hash);
    assert(8 <= MD4_DIGEST_LENGTH);
    return *(size_t *)temp_hash;
}

static uint64_t hash(const uint8_t *key, size_t len)
{
    //return noop_hash(key, len);
    //return mul_hash(key, len);
    //return tab_hash(key, len);
    //return (uint64_t)crc32_le(0xffffffff, key, len);
    //return (uint64_t)crc32_le(0xffffffff, key, len) * 18446744073709551557UL;
    //return sbox_hash(key, len);
    //return hash_md4(key, len);
    //return CityHashCrc128((const char *)key, len).first;
    return CityHash64((const char *)key, len);
}

#endif

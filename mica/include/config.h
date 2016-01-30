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

#ifndef CONFIG_H_
#define CONFIG_H_

// mehcached configuration

// be verbose (only for debugging)
//#define MEHCACHED_VERBOSE

// use counters to collect statistics
//#define MEHCACHED_COLLECT_STATS


// support for concurrent access
// #define MEHCACHED_CONCURRENT


// store mode
#define MEHCACHED_NO_EVICTION

// use log-structured pool allocator (other MEHCACHED_ALLOC_* must be undef)
//#define MEHCACHED_ALLOC_POOL
#ifndef MEHCACHED_NO_EVICTION
#define MEHCACHED_ALLOC_POOL
#endif

// use malloc allocator for each item (other MEHCACHED_ALLOC_* must be undef)
//#define MEHCACHED_ALLOC_MALLOC

// use custom dynamic allocator for each item (other MEHCACHED_ALLOC_* must be undef)
//#define MEHCACHED_ALLOC_DYNAMIC
#ifdef MEHCACHED_NO_EVICTION
#define MEHCACHED_ALLOC_DYNAMIC
#endif

#endif // CONFIG_H_

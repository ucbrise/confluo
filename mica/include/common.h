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

#ifndef COMMON_H_
#define COMMON_H_

#include "basic_types.h"

#ifdef __cplusplus
#define MEHCACHED_BEGIN extern "C" {
#define MEHCACHED_END }
#else
#define MEHCACHED_BEGIN
#define MEHCACHED_END
#endif

#define MEHCACHED_UNUSED __attribute__((unused))
#define MEHCACHED_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define MEHCACHED_ALWAYS_INLINE __attribute__((always_inline))

#define MEHCACHED_ALIGNED(alignment) __attribute__ ((aligned (alignment)))

#include "config.h"

#include <assert.h>

#endif // COMMON_H_

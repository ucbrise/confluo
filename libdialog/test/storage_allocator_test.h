#ifndef TEST_ALLOCATOR_TEST_H_
#define TEST_ALLOCATOR_TEST_H_

#include "gtest/gtest.h"
#include "memory_stat.h"
#include "storage_allocator.h"

using namespace ::dialog::storage;

class AllocatorTest : public testing::Test {

 public:
  static const size_t ARRAY_SIZE = 1024;

};

TEST_F(AllocatorTest, AllocTest) {
  storage_allocator allocator;

  int64_t start = utils::time_utils::cur_ns();
  uint64_t* ptr = allocator.alloc<uint64_t>(ARRAY_SIZE);
  int64_t stop = utils::time_utils::cur_ns();

  LOG_INFO << "Allocation latency: " << stop - start;

  for (size_t i = 0; i < ARRAY_SIZE; i++) {
    ptr[i] = i;
  }

  for (size_t i = 0; i < ARRAY_SIZE; i++) {
    ASSERT_EQ(ptr[i], i);
  }
}

TEST_F(AllocatorTest, DeallocTest) {
  storage_allocator allocator;
  size_t used_memory = allocator.memory_utilization();

  uint64_t* ptr = allocator.alloc<uint64_t>(ARRAY_SIZE);
  size_t expected_used = ARRAY_SIZE * sizeof(uint64_t) + sizeof(ptr_metadata);
  ASSERT_EQ(allocator.memory_utilization() - used_memory, expected_used);

  allocator.dealloc(ptr);
  ASSERT_EQ(allocator.memory_utilization(), used_memory);
}

TEST_F(AllocatorTest, GetMetaDataTest) {
  storage_allocator allocator;
  size_t used_memory = allocator.memory_utilization();
  uint64_t* ptr = allocator.alloc<uint64_t>(ARRAY_SIZE);

  ptr_metadata* metadata = ptr_metadata::get(ptr);
  ASSERT_EQ(metadata->size_, ARRAY_SIZE * sizeof(uint64_t));
}

#endif /* TEST_ALLOCATOR_TEST_H_ */

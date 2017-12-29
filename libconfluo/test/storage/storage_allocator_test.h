#ifndef CONFLUO_TEST_STORAGE_ALLOCATOR_TEST_H_
#define CONFLUO_TEST_STORAGE_ALLOCATOR_TEST_H_

#include "storage/storage_allocator.h"
#include "storage/memory_stat.h"
#include "gtest/gtest.h"

using namespace ::confluo::storage;

class StorageAllocatorTest : public testing::Test {

 public:
  static const size_t ARRAY_SIZE = 1024;

};

TEST_F(StorageAllocatorTest, AllocTest) {
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

TEST_F(StorageAllocatorTest, DeallocTest) {
  storage_allocator allocator;
  size_t used_memory = allocator.memory_utilization();

  uint64_t* ptr = allocator.alloc<uint64_t>(ARRAY_SIZE);
  size_t expected_used = ARRAY_SIZE * sizeof(uint64_t) + sizeof(ptr_metadata);
  ASSERT_EQ(allocator.memory_utilization() - used_memory, expected_used);

  allocator.dealloc(ptr);
  ASSERT_EQ(allocator.memory_utilization(), used_memory);
}

TEST_F(StorageAllocatorTest, GetMetaDataTest) {
  storage_allocator allocator;
  size_t used_memory = allocator.memory_utilization();
  uint64_t* ptr = allocator.alloc<uint64_t>(ARRAY_SIZE);

  ptr_metadata* metadata = ptr_metadata::get(ptr);
  ASSERT_EQ(metadata->size_, ARRAY_SIZE * sizeof(uint64_t));
}

#endif /* CONFLUO_TEST_STORAGE_ALLOCATOR_TEST_H_ */

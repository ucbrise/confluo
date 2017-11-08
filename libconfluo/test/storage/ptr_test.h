#ifndef CONFLUO_TEST_PTR_TEST_H_
#define CONFLUO_TEST_PTR_TEST_H_

#include "storage/ptr.h"
#include "gtest/gtest.h"

#include "storage/allocator.h"
#include "storage/encoded_ptr.h"
#include "storage/ptr_metadata.h"

using namespace ::confluo;
using namespace ::confluo::storage;

class PtrTest : public testing::Test {

 public:
  static const size_t ARRAY_SIZE = 1024;
  const size_t ALLOC_SIZE = sizeof(ptr_metadata) + 1024 * sizeof(ARRAY_SIZE);

};

TEST_F(PtrTest, CopyTest) {
  size_t initial_mem_usage = ALLOCATOR.memory_utilization();

  {
    void* data = ALLOCATOR.alloc(sizeof(uint64_t) * ARRAY_SIZE);
    encoded_ptr<uint64_t> enc_ptr(data);
    swappable_ptr<uint64_t> ptr(enc_ptr);

    ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
    {
      read_only_ptr<uint64_t> ptr_copy;
      ptr.atomic_copy(ptr_copy);
      ASSERT_EQ(data, ptr_copy.get().internal_ptr());
      ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
      {
        read_only_ptr<uint64_t> ptr_copy2;
        ptr.atomic_copy(ptr_copy2);
        ASSERT_EQ(data, ptr_copy2.get().internal_ptr());
        ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
      }
    }
  }

  // all pointers go out of scope, resulting in deallocation
  ASSERT_EQ(initial_mem_usage, ALLOCATOR.memory_utilization());
}


TEST_F(PtrTest, SwapTest) {
  size_t initial_mem_usage = ALLOCATOR.memory_utilization();

  {
    void* data = ALLOCATOR.alloc(sizeof(uint64_t) * ARRAY_SIZE);
    void* data_swapped = ALLOCATOR.alloc(sizeof(uint64_t) * ARRAY_SIZE);
    ptr_metadata::get(data_swapped)->state_ = state_type::D_ARCHIVED;

    encoded_ptr<uint64_t> enc_ptr(data);
    encoded_ptr<uint64_t> enc_swapped_ptr(data_swapped);

    swappable_ptr<uint64_t> ptr(enc_ptr);

    {
      read_only_ptr<uint64_t> ptr_copy;
      ptr.atomic_copy(ptr_copy);
      ASSERT_EQ(data, ptr_copy.get().internal_ptr());
      {
        read_only_ptr<uint64_t> ptr_copy2;
        ptr.atomic_copy(ptr_copy2);
        ASSERT_EQ(data, ptr_copy2.get().internal_ptr());

        // swap pointer
        ptr.swap_ptr(enc_swapped_ptr);
        ASSERT_EQ(initial_mem_usage + 2 * ALLOC_SIZE, ALLOCATOR.memory_utilization());

        read_only_ptr<uint64_t> ptr_copy3;
        ptr.atomic_copy(ptr_copy3);
        ASSERT_EQ(data_swapped, ptr_copy3.get().internal_ptr());

      }
      ASSERT_EQ(initial_mem_usage + 2 * ALLOC_SIZE, ALLOCATOR.memory_utilization());
    }

    // first internal pointer is deallocated since all readers go out of scope
    ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
  }

  // second pointer is deallocated, since the root pointer and all readers go out of scope
  ASSERT_EQ(initial_mem_usage, ALLOCATOR.memory_utilization());
}


#endif /* CONFLUO_TEST_PTR_TEST_H_ */

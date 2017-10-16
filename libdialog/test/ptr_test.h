#ifndef TEST_PTR_TEST_H_
#define TEST_PTR_TEST_H_

#include "gtest/gtest.h"

#include "dialog_allocator.h"
#include "ptr.h"
#include "ptr_metadata.h"

using namespace ::dialog;
using namespace ::dialog::memory;

class PtrTest : public testing::Test {

 public:
  static const size_t ARRAY_SIZE = 1024;
  const size_t ALLOC_SIZE = sizeof(ptr_metadata) + 1024 * sizeof(ARRAY_SIZE);

};

TEST_F(PtrTest, CopyTest) {
  size_t initial_mem_usage = ALLOCATOR.memory_utilization();

  {
    uint64_t* data = ALLOCATOR.alloc<uint64_t>(ARRAY_SIZE);
    swappable_ptr<uint64_t> ptr(data);

    ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
    {
      read_only_ptr<uint64_t> ptr_copy;
      ptr.atomic_copy(ptr_copy);
      ASSERT_EQ(data, ptr_copy.get());
      ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
      {
        read_only_ptr<uint64_t> ptr_copy2;
        ptr.atomic_copy(ptr_copy2);
        ASSERT_EQ(data, ptr_copy2.get());
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
    uint64_t* data = ALLOCATOR.alloc<uint64_t>(ARRAY_SIZE);
    uint64_t* data_swapped = ALLOCATOR.alloc<uint64_t>(ARRAY_SIZE);
    ptr_metadata::get_metadata(data_swapped)->state_ = state_type::D_ARCHIVED;
    swappable_ptr<uint64_t> ptr(data);

    {
      read_only_ptr<uint64_t> ptr_copy;
      ptr.atomic_copy(ptr_copy);
      ASSERT_EQ(data, ptr_copy.get());
      {
        read_only_ptr<uint64_t> ptr_copy2;
        ptr.atomic_copy(ptr_copy2);
        ASSERT_EQ(data, ptr_copy2.get());

        // swap pointer
        ptr.swap_ptr(data_swapped);
        ASSERT_EQ(initial_mem_usage + 2 * ALLOC_SIZE, ALLOCATOR.memory_utilization());

        read_only_ptr<uint64_t> ptr_copy3;
        ptr.atomic_copy(ptr_copy3);
        ASSERT_EQ(data_swapped, ptr_copy3.get());

      }
      ASSERT_EQ(initial_mem_usage + 2 * ALLOC_SIZE, ALLOCATOR.memory_utilization());
    }

    // first internal pointer is deallocated since all readers go out of scope
    ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
  }

  // second pointer is deallocated, since the root pointer and all readers go out of scope
  ASSERT_EQ(initial_mem_usage, ALLOCATOR.memory_utilization());
}


#endif /* TEST_PTR_TEST_H_ */

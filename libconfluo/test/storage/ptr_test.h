#ifndef CONFLUO_TEST_PTR_TEST_H_
#define CONFLUO_TEST_PTR_TEST_H_

#include "gtest/gtest.h"

#include "storage/swappable_encoded_ptr.h"
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

TEST_F(PtrTest, MetadataSizeTest) {
  ASSERT_EQ(sizeof(ptr_metadata), 8);
}

TEST_F(PtrTest, CopyTest) {
  size_t initial_mem_usage = ALLOCATOR.memory_utilization();

  {
    void *data = ALLOCATOR.alloc(sizeof(uint64_t) * ARRAY_SIZE, ptr_aux_block());
    encoded_ptr<uint64_t> enc_ptr(data);
    swappable_encoded_ptr<uint64_t> ptr(enc_ptr);

    ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
    {
      read_only_encoded_ptr<uint64_t> ptr_copy;
      ptr.atomic_copy(ptr_copy);
      ASSERT_EQ(data, ptr_copy.get().ptr());
      ASSERT_EQ(initial_mem_usage + ALLOC_SIZE, ALLOCATOR.memory_utilization());
      {
        read_only_encoded_ptr<uint64_t> ptr_copy2;
        ptr.atomic_copy(ptr_copy2);
        ASSERT_EQ(data, ptr_copy2.get().ptr());
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
    ptr_aux_block aux_unarchived(state_type::D_IN_MEMORY, encoding_type::D_UNENCODED);
    ptr_aux_block aux_archived(state_type::D_ARCHIVED, encoding_type::D_UNENCODED);

    void *data = ALLOCATOR.alloc(sizeof(uint64_t) * ARRAY_SIZE, aux_unarchived);
    void *data_swapped = ALLOCATOR.alloc(sizeof(uint64_t) * ARRAY_SIZE, aux_archived);

    encoded_ptr<uint64_t> enc_ptr(data);
    encoded_ptr<uint64_t> enc_swapped_ptr(data_swapped);

    swappable_encoded_ptr<uint64_t> ptr(enc_ptr);

    {
      read_only_encoded_ptr<uint64_t> ptr_copy;
      ptr.atomic_copy(ptr_copy);
      ASSERT_EQ(data, ptr_copy.get().ptr());
      {
        read_only_encoded_ptr<uint64_t> ptr_copy2;
        ptr.atomic_copy(ptr_copy2);
        ASSERT_EQ(data, ptr_copy2.get().ptr());

        // swap pointer
        ptr.swap_ptr(enc_swapped_ptr);
        ASSERT_EQ(initial_mem_usage + 2 * ALLOC_SIZE, ALLOCATOR.memory_utilization());

        read_only_encoded_ptr<uint64_t> ptr_copy3;
        ptr.atomic_copy(ptr_copy3);
        ASSERT_EQ(data_swapped, ptr_copy3.get().ptr());

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

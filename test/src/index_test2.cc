#include "tieredindex.h"
#include "gtest/gtest.h"

class Index2Test : public testing::Test {
 public:
  const size_t kMaxEntries = 256 * 256;

  template<typename INDEX>
  void index_test(INDEX& index, uint32_t step = 1) {
    uint32_t max = std::min(kMaxEntries, index.max_size());
    for (uint32_t i = 0; i < max; i += step) {
      index.add_entry(i, i);
    }

    for (uint32_t i = 0; i < max; i += step) {
      slog::entry_list* list = index.get(i);
      uint32_t size = list->size();
      bool found = false;
      for (uint32_t j = 0; j < size; j++) {
        found = (found || list->at(j) == i);
      }
      ASSERT_TRUE(found);
    }
  }

  size_t value_size() {
    return 24 * sizeof(std::atomic<uint64_t*>) + 16 * sizeof(uint64_t);
  }

  size_t indexlet_size(size_t size) {
    return size * (sizeof(slog::entry_list*) + value_size());
  }
};

TEST_F(Index2Test, IndexletTest) {
  struct uint32_wrapper {
    uint32_t val;

    uint32_wrapper() {
      val = 0;
    }

    size_t storage_size() {
      return sizeof(uint32_t);
    }
  };

  slog::indexlet<uint32_wrapper> ilet;

  for (uint32_t i = 0; i < ilet.size(); i++) {
    uint32_wrapper* ptr = ilet[i];
    ptr->val = i;
  }

  for (uint32_t i = 0; i < ilet.size(); i++) {
    uint32_wrapper* val = ilet.at(i);
    ASSERT_EQ(val->val, i);
  }

  size_t storage_size = ilet.storage_size();
  size_t expected_size = ilet.size()
      * (sizeof(std::atomic<uint32_t*>) + sizeof(uint32_t));
  ASSERT_EQ(expected_size, storage_size);
}

TEST_F(Index2Test, Index1GetFetchTest) {
  slog::__index1 index;

  index_test(index);
  size_t storage_size = index.storage_size();
  size_t expected_size = indexlet_size(256);
  ASSERT_EQ(expected_size, storage_size);
}

TEST_F(Index2Test, Index2GetFetchTest) {
  slog::__index2 index;

  index_test(index);
  size_t storage_size = index.storage_size();
  size_t expected_size = indexlet_size(65536);
  ASSERT_EQ(expected_size, storage_size);
}

TEST_F(Index2Test, Index3GetFetchTest) {
  slog::__index3 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = 65536 * sizeof(slog::indexlet<slog::entry_list, 256>*)
      + 256 * indexlet_size(256);
  ASSERT_EQ(expected_size, storage_size);
}

TEST_F(Index2Test, Index4GetFetchTest) {
  slog::__index4 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = 65536 * sizeof(slog::indexlet<slog::entry_list, 256>*)
      + indexlet_size(65536);
  ASSERT_EQ(expected_size, storage_size);
}

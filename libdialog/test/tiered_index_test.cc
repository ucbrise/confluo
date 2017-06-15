#include "../dialog/tiered_index.h"

#include "gtest/gtest.h"

#include <thread>

using namespace ::dialog::index;

class TieredIndexTest : public testing::Test {
 public:
  const size_t kMaxEntries = 256 * 256;

  template<typename index_type>
  void index_test_tiered(index_type& index, uint64_t step = 1) {
    uint32_t max = kMaxEntries;
    for (uint64_t i = 0; i < max; i += step) {
      index[i]->push_back(i);
    }

    for (uint64_t i = 0; i < max; i += step) {
      entry_list* list = index.at(i);
      uint32_t size = list->size();
      bool found = false;
      for (uint32_t j = 0; j < size; j++) {
        found = (found || list->at(j) == i);
      }
      ASSERT_TRUE(found);
    }
  }

  template<typename index_type>
  void index_test_tiered_mt(index_type& index, uint32_t num_threads) {
    uint32_t max = kMaxEntries;

    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, max, &index] {
        for (uint32_t j = 0; j < max; j++) {
          index[j]->push_back(i);
        }
      }));
    }

    for (std::thread& worker : workers) {
      worker.join();
    }

    for (uint32_t i = 0; i < max; i++) {
      entry_list* list = index.at(i);

      uint32_t size = list->size();
      ASSERT_EQ(num_threads, size);

      std::vector<uint32_t> counts(num_threads, 0);
      for (uint32_t j = 0; j < size; j++) {
        uint64_t val = list->at(j);
        ASSERT_TRUE(val >= 1 && val <= num_threads);
        counts[val - 1]++;
      }

      for (uint32_t count : counts) {
        ASSERT_EQ(1U, count);
      }
    }
  }

  template<typename index_type>
  void index_test(index_type& index, uint64_t step = 1) {
    uint32_t max = std::min(kMaxEntries, index.max_size());
    for (uint64_t i = 0; i < max; i += step) {
      index.add_entry(i, i);
    }

    for (uint64_t i = 0; i < max; i += step) {
      entry_list* list = index.get(i);
      uint32_t size = list->size();
      bool found = false;
      for (uint32_t j = 0; j < size; j++) {
        found = (found || list->at(j) == i);
      }
      ASSERT_TRUE(found);
    }
  }

  template<typename index_type>
  void index_test_mt(index_type& index, uint32_t num_threads) {
    uint32_t max = std::min(kMaxEntries, index.max_size());

    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, max, &index] {
        for (uint32_t j = 0; j < max; j++) {
          index.add_entry(j, i);
        }
      }));
    }

    for (std::thread& worker : workers) {
      worker.join();
    }

    for (uint32_t i = 0; i < max; i++) {
      entry_list* list = index.get(i);

      uint32_t size = list->size();
      ASSERT_EQ(num_threads, size);

      std::vector<uint32_t> counts(num_threads, 0);
      for (uint32_t j = 0; j < size; j++) {
        uint64_t val = list->at(j);
        ASSERT_TRUE(val >= 1 && val <= num_threads);
        counts[val - 1]++;
      }

      for (uint32_t count : counts) {
        ASSERT_EQ(1U, count);
      }
    }
  }

  size_t value_size() {
    return 24 * sizeof(atomic::type<uint64_t*>) + 16 * sizeof(uint64_t);
  }

  size_t indexlet_size(size_t size) {
    return size * (sizeof(entry_list*) + value_size());
  }

  template<typename T>
  size_t layer_size(uint32_t num_nodes) {
    return num_nodes * 65536 * sizeof(T*);
  }
};

TEST_F(TieredIndexTest, IndexletTest) {
  struct uint32_wrapper {
    uint32_t val;

    uint32_wrapper() {
      val = 0;
    }

    size_t storage_size() {
      return sizeof(uint32_t);
    }
  };

  indexlet<uint32_wrapper> ilet;

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
      * (sizeof(atomic::type<uint32_t*>) + sizeof(uint32_t));
  ASSERT_EQ(expected_size, storage_size);
}

TEST_F(TieredIndexTest, Index1AddFetchTest) {
  tiered_index1 index;

  index_test(index);
  size_t storage_size = index.storage_size();
  size_t expected_size = indexlet_size(256);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index1 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index2AddFetchTest) {
  tiered_index2 index;

  index_test(index);
  size_t storage_size = index.storage_size();
  size_t expected_size = indexlet_size(65536);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index2 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index3AddFetchTest) {
  tiered_index3 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = layer_size<__index_depth1 <256>>(1)
      + 256 * indexlet_size(256);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index3 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index4AddFetchTest) {
  tiered_index4 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = layer_size<__index_depth1 <65536>>(1)
      + indexlet_size(65536);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index4 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index5AddFetchTest) {
  tiered_index5 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = layer_size<__index_depth2 <65536, 256>>(1)
      + layer_size<__index_depth1 <256>>(1) + 256 * indexlet_size(256);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index5 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index6AddFetchTest) {
  tiered_index6 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = layer_size<__index_depth2 <65536, 65536>>(1)
      + layer_size<__index_depth1 <65536>>(1) + indexlet_size(65536);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index6 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index7AddFetchTest) {
  tiered_index7 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = layer_size<__index_depth3 <65536, 65536, 256>>(1)
      + layer_size<__index_depth2 <65536, 256>>(1)
      + layer_size<__index_depth1 <256>>(1) + 256 * indexlet_size(256);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index7 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, Index8AddFetchTest) {
  tiered_index8 index;

  index_test(index, 1);
  size_t storage_size = index.storage_size();
  size_t expected_size = layer_size<__index_depth3 <65536, 65536, 65536>>(1)
      + layer_size<__index_depth2 <65536, 65536>>(1)
      + layer_size<__index_depth1 <65536>>(1) + indexlet_size(65536);
  ASSERT_EQ(expected_size, storage_size);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index8 idx;
    index_test_mt(idx, num_threads);
  }
}

TEST_F(TieredIndexTest, TieredIndexAddFetchTest) {
  tiered_index<entry_list, 64, 4> index;

  index_test_tiered(index, 1);

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    tiered_index<entry_list, 64, 4> idx;
    index_test_tiered_mt(idx, num_threads);
  }
}

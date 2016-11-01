#include "tieredindex.h"
#include "gtest/gtest.h"

class Index2Test : public testing::Test {
 public:
  const size_t kMaxEntries = 256 * 256;

  template<typename INDEX>
  void index_test(INDEX& index, uint32_t step = 1) {
    uint32_t max = std::min(kMaxEntries, index.max_size());
    for (uint32_t i = 0; i < max; i+= step) {
      index.add_entry(i, i);
    }

    for (uint32_t i = 0; i < max; i+= step) {
      slog::entry_list* list = index.get(i);
      uint32_t size = list->size();
      bool found = false;
      for (uint32_t j = 0; j < size; j++) {
        found = (found || list->at(j) == i);
      }
      ASSERT_TRUE(found);
    }
  }
};

TEST_F(Index2Test, IndexletTest) {
  slog::indexlet<uint32_t> ilet;

  for (uint32_t i = 0; i < ilet.size(); i++) {
    uint32_t* ptr = ilet[i];
    *ptr = i;
  }

  for (uint32_t i = 0; i < ilet.size(); i++) {
    uint32_t* val = ilet.at(i);
    ASSERT_EQ(*val, i);
  }
}

TEST_F(Index2Test, Index1GetFetchTest) {
  slog::__index1 index;

  index_test(index);
}

TEST_F(Index2Test, Index2GetFetchTest) {
  slog::__index2 index;

  index_test(index);
}

TEST_F(Index2Test, Index3GetFetchTest) {
  slog::__index3 index;

  index_test(index, 32);
}

TEST_F(Index2Test, Index4GetFetchTest) {
  slog::__index4 index;

  index_test(index, 1024);
}

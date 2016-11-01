#include "indexlog.h"
#include "gtest/gtest.h"

class IndexTest : public testing::Test {
 public:
  const uint32_t kMaxEntries = 256 * 256;

  template<uint32_t L1, uint32_t L2>
  void index_test(slog::indexlog<L1, L2>* index) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      unsigned char* token = (unsigned char*) (&i);
      index->add_entry(token, i);
    }

    for (uint32_t i = 0; i < kMaxEntries; i++) {
      unsigned char* token = (unsigned char*) (&i);
      slog::entry_list* list = index->get_entry_list(token);
      uint32_t size = list->size();
      bool found = false;
      for (uint32_t j = 0; j < size; j++) {
        found = (found || list->at(j) == i);
      }
      ASSERT_TRUE(found);
    }
  }
};

TEST_F(IndexTest, IndexGetFetchTests) {
  slog::indexlog<4, 3> *idx43 = new slog::indexlog<4, 3>;
  slog::indexlog<4, 2> *idx42 = new slog::indexlog<4, 2>;
  slog::indexlog<3, 3> *idx33 = new slog::indexlog<3, 3>;
  slog::indexlog<3, 2> *idx32 = new slog::indexlog<3, 2>;
  slog::indexlog<2, 2> *idx22 = new slog::indexlog<2, 2>;

  index_test(idx43);
  index_test(idx42);
  index_test(idx33);
  index_test(idx32);
  index_test(idx22);
}

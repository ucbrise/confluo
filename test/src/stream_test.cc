#include "streamlog.h"
#include "gtest/gtest.h"

#include "test_utils.h"

class StreamTest : public testing::Test {
 public:
  const uint32_t kMaxEntries = 100000;

  void fill_stream(slog::streamlog& stream) {
    slog::token_list tokens;
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      stream.check_and_add(i, (unsigned char*) &i, sizeof(uint32_t), tokens);
    }
  }
};

TEST_F(StreamTest, StreamAddFetchTest1) {
  slog::streamlog stream(filter_fn1);
  fill_stream(stream);

  slog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();
  ASSERT_EQ(10000, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, list->at(i));
  }
}

TEST_F(StreamTest, StreamAddFetchTest2) {
  slog::streamlog stream(filter_fn2);
  fill_stream(stream);

  slog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();
  ASSERT_EQ(10000, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i, list->at(i));
  }
}

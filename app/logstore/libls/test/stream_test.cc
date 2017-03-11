#include "streamlog.h"
#include "test_utils.h"
#include "gtest/gtest.h"

#include <thread>

class StreamTest : public testing::Test {
 public:
  const uint32_t kMaxEntries = 100000;
  const monolog::token_list tokens;

  void fill_stream(monolog::streamlog& stream) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      stream.check_and_add(i, (unsigned char*) &i, sizeof(uint32_t), tokens);
    }
  }

  void fill_stream_mt(monolog::streamlog& stream, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &stream, this] {
        for (uint32_t j = (i - 1) * kMaxEntries; j < i * kMaxEntries; j++) {
          stream.check_and_add(j, NULL, sizeof(uint32_t), tokens);
        }
      }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
  }
};

TEST_F(StreamTest, StreamAddFetchTest1) {
  monolog::streamlog stream(filter_fn1);
  fill_stream(stream);

  monolog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();
  ASSERT_EQ(10000U, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, list->at(i));
  }

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog::streamlog s(filter_fn1);
    fill_stream_mt(s, num_threads);

    monolog::entry_list* list = s.get_stream();
    uint32_t size = list->size();
    ASSERT_EQ(10000U * num_threads, size);
    std::vector<uint32_t> counts(size);
    for (uint32_t i = 0; i < size; i++) {
      uint64_t val = list->at(i);
      ASSERT_TRUE(val % 10 == 0);
      counts[val / 10]++;
    }

    for (uint32_t count : counts) {
      ASSERT_EQ(1U, count);
    }
  }
}

TEST_F(StreamTest, StreamAddFetchTest2) {
  monolog::streamlog stream(filter_fn2);
  fill_stream(stream);

  monolog::entry_list* list = stream.get_stream();
  uint32_t size = list->size();
  ASSERT_EQ(10000U, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i, list->at(i));
  }

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    monolog::streamlog s(filter_fn2);
    fill_stream_mt(s, num_threads);

    monolog::entry_list* list = s.get_stream();
    uint32_t size = list->size();
    ASSERT_EQ(10000U * num_threads, size);
    std::vector<uint32_t> counts(size);
    for (uint32_t i = 0; i < size; i++) {
      uint64_t val = list->at(i);
      ASSERT_TRUE(val < size);
      counts[val]++;
    }

    for (uint32_t count : counts) {
      ASSERT_EQ(1U, count);
    }
  }
}

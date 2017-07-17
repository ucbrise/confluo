#ifndef TEST_FILTER_TEST_H_
#define TEST_FILTER_TEST_H_

#include "gtest/gtest.h"

#include <thread>

#include "filter.h"

using namespace ::dialog::monitor;
using namespace ::dialog::monolog;
using namespace ::dialog;

// Stateless filter
inline bool filter1(const record_t& r) {
  return r.timestamp() % 10 == 0;
}

atomic::type<uint64_t> __count(0);
uint64_t sample_mod = 10;
inline bool filter2(const record_t& r) {
  return atomic::faa(&__count, UINT64_C(1)) % sample_mod == 0;
}

class FilterTest : public testing::Test {
 public:
  const uint32_t kMaxEntries = 100000;

  struct data_point {
    uint64_t ts;
    uint64_t val;

    data_point(uint64_t _ts, uint64_t _val)
        : ts(_ts),
          val(_val) {
    }

    data_point(uint64_t _val)
        : ts(0),
          val(_val) {
    }
  };

  void fill(filter& f) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      data_point p(i, i);
      f.update(
          record_t(i, reinterpret_cast<uint8_t*>(&p) + sizeof(uint64_t),
                   sizeof(uint64_t)));
    }
  }

  void fill_mt(filter& f, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(
          std::thread(
              [i, &f, this] {
                for (uint32_t j = (i - 1) * kMaxEntries; j < i * kMaxEntries; j++) {
                  data_point p(j, j);
                  f.update(record_t(j, reinterpret_cast<uint8_t*>(&p) + sizeof(uint64_t), sizeof(uint64_t)));
                }
              }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
  }
};

TEST_F(FilterTest, AddFetchTest1) {
  filter f(filter1, 1);
  fill(f);

  reflog* s = f.get(0);
  uint32_t size = s->size();
  ASSERT_EQ(10000U, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, s->at(i));
  }

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    filter f1(filter1, 1);
    fill_mt(f1, num_threads);

    reflog* s = f1.get(0);
    uint32_t size = s->size();
    ASSERT_EQ(10000U * num_threads, size);
    std::vector<uint32_t> counts(size);
    for (uint32_t i = 0; i < size; i++) {
      uint64_t val = s->at(i);
      ASSERT_TRUE(val % 10 == 0);
      counts[val / 10]++;
    }

    for (uint32_t count : counts) {
      ASSERT_EQ(1U, count);
    }
  }
}

TEST_F(FilterTest, AddFetchTest2) {
  filter f(filter2, 1);
  fill(f);

  reflog* list = f.get(0);
  uint32_t size = list->size();
  ASSERT_EQ(10000U, size);
  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(i * 10, list->at(i));
  }

  for (uint32_t num_threads = 1; num_threads <= 4; num_threads++) {
    filter f2(filter2, 1);
    fill_mt(f2, num_threads);

    reflog* list = f2.get(0);
    uint32_t size = list->size();
    ASSERT_EQ(10000U * num_threads, size);
    for (uint32_t i = 0; i < size; i++) {
      uint64_t val = list->at(i);
      ASSERT_TRUE(val < num_threads * kMaxEntries);
    }
  }
}

#endif // TEST_FILTER_TEST_H_

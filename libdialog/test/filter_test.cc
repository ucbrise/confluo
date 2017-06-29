#include "gtest/gtest.h"

#include <thread>

#include "monitor.h"
#include "attributes.h"

using namespace ::dialog::filter;
using namespace ::dialog::monolog;
using namespace ::dialog;

// Stateless filter
bool filter1(uint64_t id, uint8_t* data, size_t len, attribute_list& attrs) {
  return id % 10 == 0;
}

atomic::type<uint64_t> count(0);
uint64_t sample_mod = 10;
bool filter2(uint64_t ts, uint8_t* data, size_t len, attribute_list& attrs) {
  return atomic::faa(&count, UINT64_C(1)) % sample_mod == 0;
}

class FilterTest : public testing::Test {
 public:
  const uint32_t kMaxEntries = 100000;
  static attribute_list attrs;

  void fill(filter& f) {
    for (uint32_t i = 0; i < kMaxEntries; i++) {
      f.update(i, i, (uint8_t*) &i, sizeof(uint32_t), attrs);
    }
  }

  void fill_mt(filter& f, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (uint32_t i = 1; i <= num_threads; i++) {
      workers.push_back(std::thread([i, &f, this] {
        for (uint32_t j = (i - 1) * kMaxEntries; j < i * kMaxEntries; j++) {
          f.update(j, j, (uint8_t*) &j, sizeof(uint32_t), attrs);
        }
      }));
    }
    for (std::thread& worker : workers) {
      worker.join();
    }
  }
};

attribute_list FilterTest::attrs;

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

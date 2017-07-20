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
  return r.timestamp() % 10000 == 0;
}

class FilterTest : public testing::Test {
 public:\

  static const uint32_t kMaxEntries = 100000;
  static const uint64_t kTimeBlock = 1e3;
  static const uint64_t kMillisecs = 1e6;

  struct data_point {
    uint64_t ts;
    long val;

    data_point(uint64_t _ts, long _val)
        : ts(_ts),
          val(_val) {
    }
  };

  void fill(filter& f) {
    for (size_t i = 0; i < kMaxEntries; i++) {
      data_point p(i * kTimeBlock, i);
      record_t r(i, reinterpret_cast<uint8_t*>(&p), sizeof(uint64_t));
      r.push_back(field_t(0, LONG_TYPE, r.data(), false, 0, 0.0));
      f.update(r);
    }
  }

  void fill_mt(filter& f, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (size_t i = 1; i <= num_threads; i++) {
      workers.push_back(
          std::thread(
              [i, &f, this] {
                size_t begin = (i - 1) * kMaxEntries, end = i * kMaxEntries;
                for (size_t j = begin; j < end; j++) {
                  data_point p((j - begin) * kTimeBlock, j);
                  record_t r(j, reinterpret_cast<uint8_t*>(&p), sizeof(uint64_t));
                  r.push_back(field_t(0, LONG_TYPE, r.data(), false, 0, 0.0));
                  f.update(record_t(j, reinterpret_cast<uint8_t*>(&p), sizeof(uint64_t)));
                }
              }));
    }

    for (std::thread& worker : workers) {
      worker.join();
    }
  }

  static compiled_expression get_expr(std::string& expr) {
    schema_builder builder;
    builder.add_column(LONG_TYPE, "value");
    schema_t<storage::in_memory> schema(".", builder.get_columns());
    return expression_compiler::compile(expr, schema);
  }
};

TEST_F(FilterTest, FilterFunctionTest) {
  filter f(filter1);
  fill(f);

  size_t accum = 0;
  for (size_t t = 0; t < 100; t++) {
    reflog const* s = f.lookup(t);
    size_t size = s->size();
    ASSERT_EQ(static_cast<size_t>(100), size);
    for (uint32_t i = accum; i < accum + size; i++) {
      ASSERT_EQ(i * 10, s->at(i - accum));
    }
    accum += size;
  }

  auto res = f.lookup_range(0, 3);
  size_t count = res.count();
  ASSERT_EQ(static_cast<size_t>(4 * 100), count);
  size_t i = 0;
  for (const uint64_t& r : res) {
    ASSERT_EQ(i * 10, r);
    i++;
  }

  for (size_t num_threads = 1; num_threads <= 4; num_threads++) {
    filter f1(filter1);
    fill_mt(f1, num_threads);

    uint64_t n_filtered_entries = (kMaxEntries * num_threads) / 10;
    std::vector<size_t> counts(n_filtered_entries, 0);
    for (size_t t = 0; t < 100; t++) {
      reflog const* s = f1.lookup(t);
      size_t size = s->size();
      ASSERT_EQ(100 * num_threads, size);
      for (uint32_t i = 0; i < size; i++) {
        uint64_t val = s->at(i);
        ASSERT_TRUE(val % 10 == 0);
        ASSERT_TRUE(val / 10 < n_filtered_entries);
        counts[val / 10]++;
      }
    }

    for (size_t count : counts) {
      ASSERT_EQ(static_cast<size_t>(1), count);
    }
  }
}

TEST_F(FilterTest, FilterExpressionTest) {
  std::string expr("value >= 50000");
  auto cexpr = get_expr(expr);
  filter f(cexpr);
  fill(f);

  for (size_t t = 0; t < 50; t++) {
    ASSERT_EQ(nullptr, f.lookup(t));
  }

  size_t accum = 50000;
  for (size_t t = 50; t < 100; t++) {
    reflog const* s = f.lookup(t);
    size_t size = s->size();
    ASSERT_EQ(static_cast<size_t>(1000), size);
    for (uint32_t i = accum; i < accum + size; i++) {
      ASSERT_EQ(i, s->at(i - accum));
    }
    accum += size;
  }

  auto res = f.lookup_range(48, 52);
  size_t count = res.count();
  ASSERT_EQ(static_cast<size_t>(3 * 1000), count);
  size_t i = 50000;
  for (const uint64_t& r : res) {
    ASSERT_EQ(i, r);
    i++;
  }
}

#endif // TEST_FILTER_TEST_H_

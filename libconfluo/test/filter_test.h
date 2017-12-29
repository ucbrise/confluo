#ifndef CONFLUO_TEST_FILTER_TEST_H_
#define CONFLUO_TEST_FILTER_TEST_H_

#include "filter.h"
#include "gtest/gtest.h"
#include <thread>

using namespace ::confluo::monitor;
using namespace ::confluo::monolog;
using namespace ::confluo;

// Stateless filter
inline bool filter1(const record_t& r) {
  return r.timestamp() % 10000 == 0;
}

class FilterTest : public testing::Test {
 public:
  static const uint32_t kMaxEntries = 100000;
  static const uint64_t kTimeBlock = 1e3;
  static const uint64_t kMillisecs = 1e6;

  struct data_point {
    int64_t ts;
    int64_t val;

    data_point(uint64_t _ts, int64_t _val)
        : ts(_ts),
          val(_val) {
    }
  }__attribute__((packed));

  void fill(filter& f) {
    ASSERT_TRUE(thread_manager::register_thread() != -1);
    for (size_t i = 0; i < kMaxEntries; i++) {
      data_point p(i * kTimeBlock, i);
      record_t r(i, reinterpret_cast<uint8_t*>(&p), sizeof(data_point));
      r.push_back(field_t(0, LONG_TYPE, r.data(), false, 0, 0.0));
      r.push_back(
          field_t(1, LONG_TYPE,
                  reinterpret_cast<char*>(r.data()) + sizeof(int64_t), false, 0,
                  0.0));
      f.update(r);
    }
    ASSERT_TRUE(thread_manager::deregister_thread() != -1);
  }

  void fill_mt(filter& f, uint32_t num_threads) {
    std::vector<std::thread> workers;
    for (size_t i = 1; i <= num_threads; i++) {
      workers.push_back(
          std::thread(
              [i, &f, this] {
                ASSERT_TRUE(thread_manager::register_thread() != -1);
                size_t begin = (i - 1) * kMaxEntries, end = i * kMaxEntries;
                for (size_t j = begin; j < end; j++) {
                  data_point p((j - begin) * kTimeBlock, j);
                  record_t r(j, reinterpret_cast<uint8_t*>(&p), sizeof(data_point));
                  r.push_back(field_t(0, LONG_TYPE, r.data(), false, 0, 0.0));
                  r.push_back(field_t(1, LONG_TYPE, reinterpret_cast<char*>(r.data()) + sizeof(int64_t), false, 0, 0.0));
                  f.update(r);
                }
                ASSERT_TRUE(thread_manager::deregister_thread() != -1);
              }));
    }

    for (std::thread& worker : workers) {
      worker.join();
    }
  }

  static compiled_expression get_expr(std::string& expr) {
    schema_builder builder;
    builder.add_column(LONG_TYPE, "value");
    schema_t schema(builder.get_columns());
    auto t = parser::parse_expression(expr);
    return parser::compile_expression(t, schema);
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

TEST_F(FilterTest, AggregateTest) {
  std::string expr("value >= 50000");
  auto cexpr = get_expr(expr);
  filter f(cexpr);
  aggregate_info *a = new aggregate_info("agg1", aggregate_type::D_MAX,
                                         LONG_TYPE, 0);
  size_t aid = f.add_aggregate(a);
  ASSERT_EQ(0, aid);
  fill(f);
  uint64_t version = kMaxEntries + sizeof(data_point);
  for (size_t t = 50; t < 100; t++) {
    const aggregated_reflog* ar = f.lookup(t);
    int64_t expected = ((t + 1) * kTimeBlock - 1) * 1000;
    ASSERT_TRUE(numeric(expected) == ar->get_aggregate(aid, version));
  }
}

TEST_F(FilterTest, MultiThreadedAggregateTest) {
  std::string expr("value >= 50000");
  auto cexpr = get_expr(expr);
  filter f(cexpr);
  aggregate_info *a = new aggregate_info("agg1", aggregate_type::D_MAX,
                                         LONG_TYPE, 0);
  size_t aid = f.add_aggregate(a);
  ASSERT_EQ(0, aid);
  fill_mt(f, 4);
  uint64_t version = 4 * kMaxEntries + sizeof(data_point);
  for (size_t t = 50; t < 100; t++) {
    const aggregated_reflog* ar = f.lookup(t);
    int64_t expected = ((t + 1) * kTimeBlock - 1) * 1000;
    ASSERT_TRUE(numeric(expected) == ar->get_aggregate(aid, version));
  }
}

#endif // CONFLUO_TEST_FILTER_TEST_H_

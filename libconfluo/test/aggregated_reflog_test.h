#ifndef CONFLUO_TEST_AGGREGATED_REFLOG_TEST_H_
#define CONFLUO_TEST_AGGREGATED_REFLOG_TEST_H_

#include "aggregated_reflog.h"

#include "gtest/gtest.h"

using namespace ::confluo;
using namespace ::confluo::monitor;

class AggregatedReflogTest : public testing::Test {
 public:
  AggregatedReflogTest() {
    sum[0] = limits::int_zero;
    for (int32_t i = 1; i <= 10; i++) {
      sum[i] = sum[i - 1] + i;
    }

    min[0] = limits::double_max;
    for (int32_t i = 1; i <= 10; i++) {
      min[i] = std::min(min[i - 1], static_cast<double>(10 - i));
    }

    max[0] = limits::double_min;
    for (int32_t i = 1; i <= 10; i++) {
      max[i] = std::max(max[i - 1], static_cast<double>(i));
    }

    cnt[0] = limits::int_zero;
    for (int32_t i = 1; i <= 10; i++) {
      cnt[i] = i;
    }
  }

  static double sum[11];
  static double min[11];
  static double max[11];
  static int64_t cnt[11];
};

double AggregatedReflogTest::sum[11];
double AggregatedReflogTest::min[11];
double AggregatedReflogTest::max[11];
int64_t AggregatedReflogTest::cnt[11];

TEST_F(AggregatedReflogTest, GetSetTest) {
  aggregate_log log;
  log.push_back(
      new aggregate_info("agg1", aggregate_manager::get_aggregator("sum"), 0));
  log.push_back(
      new aggregate_info("agg2", aggregate_manager::get_aggregator("min"), 0));
  log.push_back(
      new aggregate_info("agg3", aggregate_manager::get_aggregator("max"), 0));
  log.push_back(
      new aggregate_info("agg4", aggregate_manager::get_aggregator("count"),
                         0));
  aggregated_reflog ar(log);

  ASSERT_TRUE(numeric(limits::double_zero) == ar.get_aggregate(0, 0));
  ASSERT_TRUE(numeric(limits::double_max) == ar.get_aggregate(1, 0));
  ASSERT_TRUE(numeric(limits::double_min) == ar.get_aggregate(2, 0));
  ASSERT_TRUE(numeric(limits::ulong_zero) == ar.get_aggregate(3, 0));

  for (int i = 1; i <= 10; i++) {
    numeric value(i);
    ar.seq_update_aggregate(0, 0, value, i * 2);
    for (int j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(sum[j]) == ar.get_aggregate(0, j * 2));
  }

  for (int i = 1; i <= 10; i++) {
    numeric value(10 - i);
    ar.seq_update_aggregate(0, 1, value, i * 2);
    for (int j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(min[j]) == ar.get_aggregate(1, j * 2));
  }

  for (int i = 1; i <= 10; i++) {
    numeric value(i);
    ar.seq_update_aggregate(0, 2, value, i * 2);
    for (int j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(max[j]) == ar.get_aggregate(2, j * 2));
  }

  for (int i = 1; i <= 10; i++) {
    ar.seq_update_aggregate(0, 3, numeric(INT64_C(1)), i * 2);
    for (int j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(cnt[j]) == ar.get_aggregate(3, j * 2));
  }

  for (int j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(sum[j / 2]) == ar.get_aggregate(0, j));
  }

  for (int j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(min[j / 2]) == ar.get_aggregate(1, j));
  }

  for (int j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(max[j / 2]) == ar.get_aggregate(2, j));
  }

  for (int j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(cnt[j / 2]) == ar.get_aggregate(3, j));
  }
}

TEST_F(AggregatedReflogTest, MultiThreadedGetSetTest) {
  aggregate_log log;
  log.push_back(
      new aggregate_info("agg1", aggregate_manager::get_aggregator("sum"), 0));
  log.push_back(
      new aggregate_info("agg2", aggregate_manager::get_aggregator("min"), 0));
  log.push_back(
      new aggregate_info("agg3", aggregate_manager::get_aggregator("max"), 0));
  log.push_back(
      new aggregate_info("agg4", aggregate_manager::get_aggregator("count"),
                         0));
  aggregated_reflog ar(log);

  ASSERT_TRUE(numeric(limits::double_zero) == ar.get_aggregate(0, 0));
  ASSERT_TRUE(numeric(limits::double_max) == ar.get_aggregate(1, 0));
  ASSERT_TRUE(numeric(limits::double_min) == ar.get_aggregate(2, 0));
  ASSERT_TRUE(numeric(limits::ulong_zero) == ar.get_aggregate(3, 0));

  std::vector<std::thread> workers;
  int max_i = std::min(10, defaults::HARDWARE_CONCURRENCY);
  int max_j = max_i * 2;
  for (int i = 1; i <= max_i; i++) {
    workers.push_back(std::thread([i, &ar] {
      ar.seq_update_aggregate(i - 1, 0, numeric(i), i * 2);
    }));
  }

  for (int i = 1; i <= max_i; i++) {
    workers.push_back(std::thread([i, &ar] {
      ar.seq_update_aggregate(i - 1, 1, numeric(10 - i), i * 2);
    }));
  }

  for (int i = 1; i <= max_i; i++) {
    workers.push_back(std::thread([i, &ar] {
      ar.seq_update_aggregate(i - 1, 2, numeric(i), i * 2);
    }));
  }

  for (int i = 1; i <= max_i; i++) {
    workers.push_back(std::thread([i, &ar] {
      ar.seq_update_aggregate(i - 1, 3, numeric(INT64_C(1)), i * 2);
    }));
  }

  for (auto& w : workers) {
    if (w.joinable())
      w.join();
  }

  for (int j = 0; j <= max_j; j++) {
    ASSERT_TRUE(numeric(sum[j / 2]) == ar.get_aggregate(0, j));
  }

  for (int j = 0; j <= max_j; j++) {
    ASSERT_TRUE(numeric(min[j / 2]) == ar.get_aggregate(1, j));
  }

  for (int j = 0; j <= max_j; j++) {
    ASSERT_TRUE(numeric(max[j / 2]) == ar.get_aggregate(2, j));
  }

  for (int j = 0; j <= max_j; j++) {
    ASSERT_TRUE(numeric(cnt[j / 2]) == ar.get_aggregate(3, j));
  }
}

#endif /* CONFLUO_TEST_AGGREGATED_REFLOG_TEST_H_ */

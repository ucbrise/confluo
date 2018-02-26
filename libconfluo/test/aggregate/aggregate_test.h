#ifndef CONFLUO_TEST_AGGREGATE_TEST_H_
#define CONFLUO_TEST_AGGREGATE_TEST_H_

#include "aggregate/aggregate.h"

#include "gtest/gtest.h"

using namespace ::confluo;

class AggregateTest : public testing::Test {
};

TEST_F(AggregateTest, SumTest) {
  aggregate_list agg(INT_TYPE, aggregate_manager::get_aggregator("sum"));
  ASSERT_TRUE(numeric(limits::double_zero) == agg.get(0));

  double sum[11];
  sum[0] = limits::double_zero;
  for (int32_t i = 1; i <= 10; i++) {
    sum[i] = sum[i - 1] + i;
  }

  for (int32_t i = 1; i <= 10; i++) {
    numeric value(i);
    agg.seq_update(value, i * 2);
    for (int32_t j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(sum[j]) == agg.get(j * 2));
  }

  for (int32_t j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(sum[j / 2]) == agg.get(j));
  }
}

TEST_F(AggregateTest, MinTest) {
  aggregate_list agg(INT_TYPE, aggregate_manager::get_aggregator("min"));
  ASSERT_TRUE(numeric(limits::double_max) == agg.get(0));

  double min[11];
  min[0] = limits::double_max;
  for (int32_t i = 1; i <= 10; i++) {
    min[i] = std::min(min[i - 1], static_cast<double>(10 - i));
  }

  for (int32_t i = 1; i <= 10; i++) {
    numeric value(10 - i);
    agg.seq_update(value, i * 2);
    for (int32_t j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(min[j]) == agg.get(j * 2));
  }

  for (int32_t j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(min[j / 2]) == agg.get(j));
  }
}

TEST_F(AggregateTest, MaxTest) {
  aggregate_list agg(INT_TYPE, aggregate_manager::get_aggregator("max"));
  ASSERT_TRUE(numeric(limits::double_min) == agg.get(0));

  double max[11];
  max[0] = limits::double_min;
  for (int32_t i = 1; i <= 10; i++) {
    max[i] = std::max(max[i - 1], static_cast<double>(i));
  }

  for (int32_t i = 1; i <= 10; i++) {
    numeric value(i);
    agg.seq_update(value, i * 2);
    for (int32_t j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(max[j]) == agg.get(j * 2));
  }

  for (int32_t j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(max[j / 2]) == agg.get(j));
  }
}

TEST_F(AggregateTest, CountTest) {
  aggregate_list agg(INT_TYPE, aggregate_manager::get_aggregator("count"));
  ASSERT_TRUE(numeric(limits::ulong_zero) == agg.get(0));

  int32_t count[11];
  count[0] = limits::int_zero;
  for (int32_t i = 1; i <= 10; i++) {
    count[i] = i;
  }

  for (int32_t i = 1; i <= 10; i++) {
    agg.seq_update(numeric(1), i * 2);
    for (int32_t j = 0; j <= i; j++)
      ASSERT_TRUE(numeric(count[j]) == agg.get(j * 2));
  }

  for (int32_t j = 0; j <= 20; j++) {
    ASSERT_TRUE(numeric(count[j / 2]) == agg.get(j));
  }
}

#endif /* CONFLUO_TEST_AGGREGATE_TEST_H_ */

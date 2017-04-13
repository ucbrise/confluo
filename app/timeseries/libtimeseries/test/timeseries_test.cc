#define GTEST_HAS_TR1_TUPLE 0

#include "timeseries_db.h"
#include "gtest/gtest.h"

#include <thread>

using namespace timeseries;
using namespace datastore;

class TimeseriesTest : public testing::Test {
 public:
  static const uint64_t kBatchSize = 1000;
  static const uint64_t kTSMax = 10000;

  std::vector<data_pt> get_pts(uint64_t start_ts) {
    std::vector<data_pt> pts;
    for (uint64_t i = 0; i < kBatchSize; i++) {
      data_pt p;
      p.timestamp = start_ts + i;
      p.value = start_ts + i;
      pts.push_back(p);
    }
    return pts;
  }
};

const uint64_t TimeseriesTest::kBatchSize;
const uint64_t TimeseriesTest::kTSMax;

TEST_F(TimeseriesTest, AddGetTest) {
  timeseries_t<> ts;
  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts = get_pts(i);
    ts.insert_values(&pts[0], pts.size());
  }

  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts;
    ts.get_range_latest(pts, i, i + kBatchSize - 1);
    ASSERT_EQ(pts.size(), kBatchSize);
    for (size_t j = 0; j < pts.size(); j++) {
      ASSERT_EQ(pts[j].timestamp, static_cast<timestamp_t>(i + j));
      ASSERT_EQ(pts[j].value, static_cast<value_t>(i + j));
    }
  }
}

TEST_F(TimeseriesTest, AddGetNearestTest) {
  timeseries_t<> ts;
  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts = get_pts(i);
    ts.insert_values(&pts[0], pts.size());
  }

  for (uint64_t i = 0; i < kTSMax; i++) {
    data_pt pt = ts.get_nearest_value_latest(true, i);
    ASSERT_EQ(pt.timestamp, static_cast<timestamp_t>(i));
    ASSERT_EQ(pt.value, static_cast<version_t>(i));

    pt = ts.get_nearest_value_latest(false, i);
    ASSERT_EQ(pt.timestamp, static_cast<timestamp_t>(i));
    ASSERT_EQ(pt.value, static_cast<version_t>(i));
  }
}

TEST_F(TimeseriesTest, AddGetStatsTest) {
  timeseries_t<256, 7> ts;

  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts = get_pts(i);
    ts.insert_values(&pts[0], pts.size());
  }

  int64_t sum = 0;
  for (uint64_t i = 0; i < kTSMax; i++)
    sum += i;

  std::vector<stats> res1;
  ts.get_statistical_range_latest(res1, 0, 65536, 16);

  ASSERT_EQ(1U, res1.size());
  ASSERT_EQ(static_cast<value_t>(kTSMax), res1[0].count);
  ASSERT_EQ(static_cast<value_t>(0), res1[0].min);
  ASSERT_EQ(static_cast<value_t>(kTSMax - 1), res1[0].max);
  ASSERT_EQ(static_cast<value_t>(sum), res1[0].sum);

  std::vector<stats> res2;
  ts.get_statistical_range_latest(res2, 0, 65536, 15);

  ASSERT_EQ(2U, res2.size());
  ASSERT_EQ(static_cast<value_t>(kTSMax), res2[0].count);
  ASSERT_EQ(static_cast<value_t>(0), res2[0].min);
  ASSERT_EQ(static_cast<value_t>(kTSMax - 1), res2[0].max);
  ASSERT_EQ(static_cast<value_t>(sum), res2[0].sum);

  ASSERT_EQ(static_cast<value_t>(0), res2[1].count);
  ASSERT_EQ(static_cast<value_t>(std::numeric_limits<value_t>::max()), res2[1].min);
  ASSERT_EQ(static_cast<value_t>(0), res2[1].max);
  ASSERT_EQ(static_cast<value_t>(0), res2[1].sum);
}

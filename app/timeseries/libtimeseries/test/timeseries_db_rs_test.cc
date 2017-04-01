#define GTEST_HAS_TR1_TUPLE 0

#include "timeseries_db.h"
#include "gtest/gtest.h"

#include <thread>

using namespace timeseries;
using namespace datastore;

class TimeseriesDBRSTest : public testing::Test {
 public:
  static const uint64_t kBatchSize = 1000;
  static const uint64_t kTSMax = 1000000;

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

const uint64_t TimeseriesDBRSTest::kBatchSize;
const uint64_t TimeseriesDBRSTest::kTSMax;

TEST_F(TimeseriesDBRSTest, AddGetTest) {
  timeseries_db_rs<> tsdb;
  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts = get_pts(i);
    tsdb.insert_values(&pts[0], pts.size());
  }

  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts;
    tsdb.get_range_latest(pts, i, i + kBatchSize - 1);
    ASSERT_EQ(pts.size(), kBatchSize);
    for (size_t j = 0; j < pts.size(); j++) {
      ASSERT_EQ(pts[j].timestamp, static_cast<timestamp_t>(i + j));
      ASSERT_EQ(pts[j].value, static_cast<value_t>(i + j));
    }
  }
}

TEST_F(TimeseriesDBRSTest, AddGetNearestTest) {
  timeseries_db_rs<> tsdb;
  for (uint64_t i = 0; i < kTSMax; i += kBatchSize) {
    std::vector<data_pt> pts = get_pts(i);
    tsdb.insert_values(&pts[0], pts.size());
  }

  for (uint64_t i = 0; i < kTSMax; i++) {
    data_pt pt = tsdb.get_nearest_value_latest(true, i);
    ASSERT_EQ(pt.timestamp, static_cast<timestamp_t>(i));
    ASSERT_EQ(pt.value, static_cast<version_t>(i));

    pt = tsdb.get_nearest_value_latest(false, i);
    ASSERT_EQ(pt.timestamp, static_cast<timestamp_t>(i));
    ASSERT_EQ(pt.value, static_cast<version_t>(i));
  }
}

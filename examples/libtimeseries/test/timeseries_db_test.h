#ifndef EXAMPLES_TEST_TIMESERIES_DB_TEST_H_
#define EXAMPLES_TEST_TIMESERIES_DB_TEST_H_

#include "timeseries_db.h"

#include "math.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;

class TimeseriesDBTest : public testing::Test {
 public:
  static task_pool MGMT_POOL;
  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_append_and_get(atomic_multilog& mlog) {
    std::vector<uint64_t> offsets;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      TimeseriesDBTest::generate_bytes(data_, DATA_SIZE, i);
      uint64_t offset = mlog.append(data_);
      offsets.push_back(offset);
    }

    record_t r;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      std::unique_ptr<uint8_t> ptr = mlog.read_raw(offsets[i]);
      ASSERT_TRUE(ptr != nullptr);
      uint8_t expected = i % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ptr.get()[j], expected);
      }
    }
    ASSERT_EQ(MAX_RECORDS, mlog.num_records());
  }

  static std::vector<column_t> s;
  static int64_t time_count;

  struct rec {
    int64_t ts;
    bool a;
    int8_t b;
    int16_t c;
    int32_t d;
    int64_t e;
    float f;
    double g;
    char h[16];
  }__attribute__((packed));

  static rec r;
  static char test_str[16];

  static char* test_string(const char* str) {
    size_t len = std::min(static_cast<size_t>(16), strlen(str));
    memcpy(test_str, str, len);
    for (size_t i = len; i < 16; i++) {
      test_str[i] = '\0';
    }
    return test_str;
  }

  static void* record(bool a, int8_t b, int16_t c, int32_t d, int64_t e,
                      float f, double g, const char* h) {
    int64_t ts = utils::time_utils::cur_ns();
    r = {ts, a, b, c, d, e, f, g, {}};
    size_t len = std::min(static_cast<size_t>(16), strlen(h));
    memcpy(r.h, h, len);
    for (size_t i = len; i < 16; i++) {
      r.h[i] = '\0';
    }
    return reinterpret_cast<void*>(&r);
  }

  static std::vector<column_t> schema() {
    schema_builder builder;
    builder.add_column(BOOL_TYPE, "a");
    builder.add_column(CHAR_TYPE, "b");
    builder.add_column(SHORT_TYPE, "c");
    builder.add_column(INT_TYPE, "d");
    builder.add_column(LONG_TYPE, "e");
    builder.add_column(FLOAT_TYPE, "f");
    builder.add_column(DOUBLE_TYPE, "g");
    builder.add_column(STRING_TYPE(16), "h");
    return builder.get_columns();
  }

  static record_batch get_batch(const atomic_multilog& mlog) {
    record_batch_builder builder = mlog.get_batch_builder();
    builder.add_record(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    builder.add_record(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
    builder.add_record(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
    builder.add_record(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
    builder.add_record(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
    builder.add_record(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
    builder.add_record(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
    builder.add_record(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
    return builder.get_batch();
  }

 protected:
  uint8_t data_[DATA_SIZE];

  virtual void SetUp() override {
    thread_manager::register_thread();
  }

  virtual void TearDown() override {
    thread_manager::deregister_thread();
  }
};

TimeseriesDBTest::rec TimeseriesDBTest::r;
std::vector<column_t> TimeseriesDBTest::s = schema();
task_pool TimeseriesDBTest::MGMT_POOL;

int64_t get_time(uint8_t* data) {
  return static_cast<int64_t>(data[0]) | static_cast<int64_t>(data[1]) << 8
      | static_cast<int64_t>(data[2]) << 16
      | static_cast<int64_t>(data[3]) << 24
      | static_cast<int64_t>(data[4]) << 32
      | static_cast<int64_t>(data[5]) << 40
      | static_cast<int64_t>(data[6]) << 48
      | static_cast<int64_t>(data[7]) << 56;
}

TEST_F(TimeseriesDBTest, AppendTest) {
  timeseries_db ts("my_multilog", s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  ts.add_index("a");
  ts.add_index("b");
  ts.add_index("c", 10);
  ts.add_index("d", 2);
  ts.add_index("e", 100);
  ts.add_index("f", 0.1);
  ts.add_index("g", 0.01);
  ts.add_index("h");

  size_t offset = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t time = r.ts;

  size_t offset1 = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t time1 = r.ts;

  size_t offset2 = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t time2 = r.ts;

  size_t offset3 = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t end = r.ts;

  std::unique_ptr<uint8_t> ptr = ts.read_raw(offset);
  uint8_t* data = ptr.get();
  // 64 bits = 8 bytes
  int64_t calculated = get_time(data);
  ASSERT_EQ(time, calculated);

  ptr = ts.read_raw(offset1);
  data = ptr.get();
  calculated = get_time(data);
  ASSERT_EQ(time1, calculated);

  ptr = ts.read_raw(offset2);
  data = ptr.get();
  calculated = get_time(data);
  ASSERT_EQ(time2, calculated);

  ptr = ts.read_raw(offset3);
  data = ptr.get();
  calculated = get_time(data);
  ASSERT_EQ(end, calculated);
}

TEST_F(TimeseriesDBTest, GetRangeTest) {
  timeseries_db ts("my_multilog", s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  ts.add_index("a");
  ts.add_index("b");
  ts.add_index("c", 10);
  ts.add_index("d", 2);
  ts.add_index("e", 100);
  ts.add_index("f", 0.1);
  ts.add_index("g", 0.01);
  ts.add_index("h");

  int64_t beg = r.ts;

  size_t offset = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t time = r.ts;

  size_t offset1 = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t time1 = r.ts;

  size_t offset2 = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t time2 = r.ts;

  size_t offset3 = ts.append(record(true, '1', 3, 5, 12, 0.5, 0.01, "abc"));
  int64_t end = r.ts;

  size_t num_pts = 4;
  std::vector<record_t> records;
  ts.get_range(records, beg, end);

  ASSERT_EQ(num_pts, records.size());
}

TEST_F(TimeseriesDBTest, GetNearestTest) {
  timeseries_db ts1("my_multilog", s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  ts1.append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  int64_t beg = r.ts;
  ts1.append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  int64_t expected = r.ts;
  ts1.append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  ts1.append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  ts1.append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  ts1.append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  ts1.append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  ts1.append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
  int64_t end = r.ts;

  record_t nearest_val = ts1.get_nearest_value(beg, true);
  ASSERT_EQ(expected, nearest_val.at(0).value().to_data().as<int64_t>());
}

TEST_F(TimeseriesDBTest, ComputeDiffTest) {

  std::vector<record_t> records;
  std::vector<uint64_t> offsets;

  timeseries_db ts1("my_multilog", s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  ts1.append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  int64_t from_version = ts1.get_version();
  offsets.push_back(ts1.append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg")));
  offsets.push_back(
      ts1.append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl")));
  offsets.push_back(
      ts1.append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr")));
  offsets.push_back(
      ts1.append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx")));
  offsets.push_back(
      ts1.append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy")));
  offsets.push_back(
      ts1.append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz")));
  offsets.push_back(
      ts1.append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz")));

  int64_t to_version = ts1.get_version();
  size_t expected_size = 7;
  ts1.compute_diff(records, from_version, to_version);
  ASSERT_EQ(expected_size, records.size());
}

#endif /* EXAMPLES_TEST_TIMESERIES_DB_TEST_H_ */

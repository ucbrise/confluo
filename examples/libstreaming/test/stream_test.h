#ifndef EXAMPLES_TEST_STREAM_TEST_H_
#define EXAMPLES_TEST_STREAM_TEST_H_

#include "stream_consumer.h"
#include "stream_producer.h"

#include "math.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;

class StreamTest : public testing::Test {
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
      StreamTest::generate_bytes(data_, DATA_SIZE, i);
      uint64_t offset = mlog.append(data_);
      offsets.push_back(offset);
    }

    record_t r;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      ro_data_ptr data_ptr;
      mlog.read(offsets[i], data_ptr);
      ASSERT_TRUE(data_ptr.get() != nullptr);
      uint8_t expected = i % 256;
      uint8_t* data = data_ptr.get();
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(data[j], expected);
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

  static std::string record_str(bool a, int8_t b, int16_t c, int32_t d,
                                int64_t e, float f, double g, const char* h) {
    void* rbuf = record(a, b, c, d, e, f, g, h);
    return std::string(reinterpret_cast<const char*>(rbuf), sizeof(rec));
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

  static record_batch get_batch() {
    record_batch_builder builder;
    builder.add_record(record_str(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    builder.add_record(record_str(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
    builder.add_record(record_str(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
    builder.add_record(record_str(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
    builder.add_record(
        record_str(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
    builder.add_record(record_str(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
    builder.add_record(
        record_str(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
    builder.add_record(
        record_str(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
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

StreamTest::rec StreamTest::r;
std::vector<column_t> StreamTest::s = schema();
task_pool StreamTest::MGMT_POOL;

int64_t get_time(uint8_t* data) {
  return static_cast<int64_t>(data[0]) | static_cast<int64_t>(data[1]) << 8
      | static_cast<int64_t>(data[2]) << 16
      | static_cast<int64_t>(data[3]) << 24
      | static_cast<int64_t>(data[4]) << 32
      | static_cast<int64_t>(data[5]) << 40
      | static_cast<int64_t>(data[6]) << 48
      | static_cast<int64_t>(data[7]) << 56;
}

TEST_F(StreamTest, WriteReadTest) {
  atomic_multilog dtable("my_table", s, "/tmp", 
          storage::IN_MEMORY, MGMT_POOL);
  
  dtable.add_index("a");
  dtable.add_index("b");
  dtable.add_index("c", 1);
  dtable.add_index("d", 2);
  dtable.add_index("e", 100);
  dtable.add_index("f", 0.1);
  dtable.add_index("g", 0.01);

  stream_producer sp(&dtable);
  stream_consumer sc(&dtable);

  std::vector<std::string> expected_strings;
  uint64_t k_max = 100000;

  for (uint64_t i = 0; i < k_max; i++) {
    std::string record_string = record_str(true, '7', i, 14, 1000, 
            0.7, 0.02, "stream");
    sp.buffer(record_string);
    expected_strings.push_back(record_string);
  }

  std::vector<size_t> log_offs = sp.get_log_offsets();

  for (uint64_t i = 0; i < k_max; i++) {
    std::string data;
    sc.read(data, i * sizeof(rec), sizeof(rec));
    ASSERT_STREQ(expected_strings[i].c_str(), data.c_str());
  }

}

#endif /* EXAMPLES_TEST_STREAM_TEST_H_ */

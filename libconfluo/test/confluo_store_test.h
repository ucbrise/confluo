#ifndef CONFLUO_TEST_CONFLUO_STORE_TEST_H_
#define CONFLUO_TEST_CONFLUO_STORE_TEST_H_

#include "confluo_store.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;

class ConfluoStoreTest : public testing::Test {
 public:
  static task_pool MGMT_POOL;
  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_append_and_get(atomic_multilog& dtable) {
    std::vector<uint64_t> offsets;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      ConfluoStoreTest::generate_bytes(data_, DATA_SIZE, i);
      uint64_t offset = dtable.append(data_);
      offsets.push_back(offset);
    }

    record_t r;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      std::unique_ptr<uint8_t> ptr = dtable.read_raw(offsets[i]);
      ASSERT_TRUE(ptr.get() != nullptr);
      uint8_t expected = i % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ptr.get()[j], expected);
      }
    }
    ASSERT_EQ(MAX_RECORDS, dtable.num_records());
  }

  static std::vector<column_t> s;

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

  static record_batch get_batch() {
    record_batch_builder builder(s);
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

ConfluoStoreTest::rec ConfluoStoreTest::r;
std::vector<column_t> ConfluoStoreTest::s = schema();
task_pool ConfluoStoreTest::MGMT_POOL;

TEST_F(ConfluoStoreTest, AddTableTest) {
  confluo_store store("/tmp");
  int64_t id = store.create_atomic_multilog("my_table", s,
                                            storage::storage_mode::IN_MEMORY);
  ASSERT_EQ(id, store.get_atomic_multilog_id("my_table"));
}

TEST_F(ConfluoStoreTest, RemoveTableTest) {
  confluo_store store("/tmp");
  int64_t id = store.create_atomic_multilog("my_table", s,
                                            storage::storage_mode::IN_MEMORY);
  ASSERT_NE(-1, store.remove_atomic_multilog(id));
  try {
    store.remove_atomic_multilog("my_table");
  } catch (std::exception& e) {
    ASSERT_STREQ("No such atomic multilog my_table", e.what());
  }
}

#endif /* CONFLUO_TEST_CONFLUO_STORE_TEST_H_ */

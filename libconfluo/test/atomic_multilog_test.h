#ifndef CONFLUO_TEST_ATOMIC_MULTILOG_TEST_H_
#define CONFLUO_TEST_ATOMIC_MULTILOG_TEST_H_

#include "atomic_multilog.h"

#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;

class AtomicMultilogTest : public testing::Test {
 public:
  static task_pool MGMT_POOL;
  static void generate_bytes(uint8_t *buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_append_and_get(atomic_multilog &mlog) {
    std::vector<uint64_t> offsets;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      AtomicMultilogTest::generate_bytes(data_, DATA_SIZE, i);
      uint64_t offset = mlog.append(data_);
      offsets.push_back(offset);
    }

    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      read_only_data_log_ptr ptr;
      mlog.read(offsets[i], ptr);
      ASSERT_TRUE(ptr.get().ptr() != nullptr);
      uint8_t expected = static_cast<uint8_t>(i % 256);
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ptr[j], expected);
      }
    }
    ASSERT_EQ(MAX_RECORDS, mlog.num_records());
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

  static void *record(bool a, int8_t b, int16_t c, int32_t d, int64_t e, float f, double g, const char *h) {
    int64_t ts = utils::time_utils::cur_ns();
    r = {ts, a, b, c, d, e, f, g, {}};
    size_t len = std::min(static_cast<size_t>(16), strlen(h));
    memcpy(r.h, h, len);
    for (size_t i = len; i < 16; i++) {
      r.h[i] = '\0';
    }
    return reinterpret_cast<void *>(&r);
  }

  static void *record(int64_t ts, bool a, int8_t b, int16_t c, int32_t d, int64_t e, float f, double g, const char *h) {
    r = {ts, a, b, c, d, e, f, g, {}};
    size_t len = std::min(static_cast<size_t>(16), strlen(h));
    memcpy(r.h, h, len);
    for (size_t i = len; i < 16; i++) {
      r.h[i] = '\0';
    }
    return reinterpret_cast<void *>(&r);
  }

  static std::vector<column_t> schema() {
    schema_builder builder;
    builder.add_column(primitive_types::BOOL_TYPE(), "a");
    builder.add_column(primitive_types::CHAR_TYPE(), "b");
    builder.add_column(primitive_types::SHORT_TYPE(), "c");
    builder.add_column(primitive_types::INT_TYPE(), "d");
    builder.add_column(primitive_types::LONG_TYPE(), "e");
    builder.add_column(primitive_types::FLOAT_TYPE(), "f");
    builder.add_column(primitive_types::DOUBLE_TYPE(), "g");
    builder.add_column(primitive_types::STRING_TYPE(16), "h");
    return builder.get_columns();
  }

  static record_batch build_batch(const atomic_multilog &mlog) {
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

  static record_batch build_batch(const atomic_multilog &mlog, int64_t ts) {
    record_batch_builder builder = mlog.get_batch_builder();
    builder.add_record(record(ts, false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    builder.add_record(record(ts, true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
    builder.add_record(record(ts, false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
    builder.add_record(record(ts, true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
    builder.add_record(record(ts, false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
    builder.add_record(record(ts, true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
    builder.add_record(record(ts, false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
    builder.add_record(record(ts, true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
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

AtomicMultilogTest::rec AtomicMultilogTest::r;
std::vector<column_t> AtomicMultilogTest::s = schema();
task_pool AtomicMultilogTest::MGMT_POOL;

TEST_F(AtomicMultilogTest, AppendAndGetInMemoryTest) {
  atomic_multilog mlog(
      "my_table",
      schema_builder().add_column(primitive_types::STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  test_append_and_get(mlog);
}

TEST_F(AtomicMultilogTest, AppendAndGetDurableTest) {
  atomic_multilog mlog(
      "my_table",
      schema_builder().add_column(primitive_types::STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::DURABLE, archival_mode::OFF, MGMT_POOL);
  test_append_and_get(mlog);
}

TEST_F(AtomicMultilogTest, AppendAndGetDurableRelaxedTest) {
  atomic_multilog mlog(
      "my_table",
      schema_builder().add_column(primitive_types::STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::DURABLE_RELAXED, archival_mode::OFF, MGMT_POOL);
  test_append_and_get(mlog);
}

TEST_F(AtomicMultilogTest, AppendAndGetJSONRecordTest1) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);

  std::string rec1 = "{'a':'false', 'b':'0', 'c':'0', 'd':'0', 'e':'0', 'f':'0.000000', 'g':'0.010000', 'h':'abc'}";
  std::string rec2 = "{'a':'true', 'b':'1', 'c':'10', 'd':'2', 'e':'10', 'f':'0.100000', 'g':'0.020000', 'h':'defg'}";
  std::string rec3 = "{'a':'false', 'b':'2', 'c':'20', 'd':'4', 'e':'100', 'f':'0.200000', 'g':'0.030000', 'h':'hijkl'}";
  std::string rec4 = "{'a':'true', 'b':'3', 'c':'30', 'd':'6', 'e':'1000', 'f':'0.300000', 'g':'0.040000', 'h':'mnopqr'}";
  std::string rec5 = "{'a':'false', 'b':'4', 'c':'40', 'd':'8', 'e':'10000', 'f':'0.400000', 'g':'0.050000', 'h':'stuvwx'}";
  std::string rec6 = "{'a':'true', 'b':'5', 'c':'50', 'd':'10', 'e':'100000', 'f':'0.500000', 'g':'0.060000', 'h':'yyy'}";
  std::string rec7 = "{'a':'false', 'b':'6', 'c':'60', 'd':'12', 'e':'1000000', 'f':'0.600000', 'g':'0.070000', 'h':'zzz'}";
  std::string rec8 = "{'a':'true', 'b':'7', 'c':'70', 'd':'14', 'e':'10000000', 'f':'0.700000', 'g':'0.080000', 'h':'zzz'}";

  ASSERT_EQ(mlog.record_size() * 0, mlog.append_json(rec1));
  ASSERT_EQ(mlog.record_size() * 1, mlog.append_json(rec2));
  ASSERT_EQ(mlog.record_size() * 2, mlog.append_json(rec3));
  ASSERT_EQ(mlog.record_size() * 3, mlog.append_json(rec4));
  ASSERT_EQ(mlog.record_size() * 4, mlog.append_json(rec5));
  ASSERT_EQ(mlog.record_size() * 5, mlog.append_json(rec6));
  ASSERT_EQ(mlog.record_size() * 6, mlog.append_json(rec7));
  ASSERT_EQ(mlog.record_size() * 7, mlog.append_json(rec8));

  std::string res1 = mlog.read_json(mlog.record_size() * 0);
  std::string res2 = mlog.read_json(mlog.record_size() * 1);
  std::string res3 = mlog.read_json(mlog.record_size() * 2);
  std::string res4 = mlog.read_json(mlog.record_size() * 3);
  std::string res5 = mlog.read_json(mlog.record_size() * 4);
  std::string res6 = mlog.read_json(mlog.record_size() * 5);
  std::string res7 = mlog.read_json(mlog.record_size() * 6);
  std::string res8 = mlog.read_json(mlog.record_size() * 7);

  ASSERT_EQ(rec1, res1);
  ASSERT_EQ(rec2, res2);
  ASSERT_EQ(rec3, res3);
  ASSERT_EQ(rec4, res4);
  ASSERT_EQ(rec5, res5);
  ASSERT_EQ(rec6, res6);
  ASSERT_EQ(rec7, res7);
  ASSERT_EQ(rec8, res8);
}

TEST_F(AtomicMultilogTest, AppendAndGetRecordTest1) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);

  typedef std::vector<std::string> rec_vector;
  rec_vector rec1{"false", "0", "0", "0", "0", "0.000000", "0.010000", "abc"};
  rec_vector rec2{"true", "1", "10", "2", "1", "0.100000", "0.020000", "defg"};
  rec_vector rec3{"false", "2", "20", "4", "10", "0.200000", "0.030000", "hijkl"};
  rec_vector rec4{"true", "3", "30", "6", "100", "0.300000", "0.040000", "mnopqr"};
  rec_vector rec5{"false", "4", "40", "8", "1000", "0.400000", "0.050000", "stuvwx"};
  rec_vector rec6{"true", "5", "50", "10", "10000", "0.500000", "0.060000", "yyy"};
  rec_vector rec7{"false", "6", "60", "12", "100000", "0.600000", "0.070000", "zzz"};
  rec_vector rec8{"true", "7", "70", "14", "1000000", "0.700000", "0.080000", "zzz"};

  ASSERT_EQ(mlog.record_size() * 0, mlog.append(rec1));
  ASSERT_EQ(mlog.record_size() * 1, mlog.append(rec2));
  ASSERT_EQ(mlog.record_size() * 2, mlog.append(rec3));
  ASSERT_EQ(mlog.record_size() * 3, mlog.append(rec4));
  ASSERT_EQ(mlog.record_size() * 4, mlog.append(rec5));
  ASSERT_EQ(mlog.record_size() * 5, mlog.append(rec6));
  ASSERT_EQ(mlog.record_size() * 6, mlog.append(rec7));
  ASSERT_EQ(mlog.record_size() * 7, mlog.append(rec8));

  rec_vector res1 = mlog.read(mlog.record_size() * 0);
  rec_vector res2 = mlog.read(mlog.record_size() * 1);
  rec_vector res3 = mlog.read(mlog.record_size() * 2);
  rec_vector res4 = mlog.read(mlog.record_size() * 3);
  rec_vector res5 = mlog.read(mlog.record_size() * 4);
  rec_vector res6 = mlog.read(mlog.record_size() * 5);
  rec_vector res7 = mlog.read(mlog.record_size() * 6);
  rec_vector res8 = mlog.read(mlog.record_size() * 7);

  ASSERT_EQ(rec1, rec_vector(res1.begin() + 1, res1.end()));
  ASSERT_EQ(rec2, rec_vector(res2.begin() + 1, res2.end()));
  ASSERT_EQ(rec3, rec_vector(res3.begin() + 1, res3.end()));
  ASSERT_EQ(rec4, rec_vector(res4.begin() + 1, res4.end()));
  ASSERT_EQ(rec5, rec_vector(res5.begin() + 1, res5.end()));
  ASSERT_EQ(rec6, rec_vector(res6.begin() + 1, res6.end()));
  ASSERT_EQ(rec7, rec_vector(res7.begin() + 1, res7.end()));
  ASSERT_EQ(rec8, rec_vector(res8.begin() + 1, res8.end()));
}

TEST_F(AtomicMultilogTest, AppendAndGetRecordTest2) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);

  typedef std::vector<std::string> rec_vector;
  rec_vector rec1{"0", "false", "0", "0", "0", "0", "0.000000", "0.010000", "abc"};
  rec_vector rec2{"0", "true", "1", "10", "2", "1", "0.100000", "0.020000", "defg"};
  rec_vector rec3{"0", "false", "2", "20", "4", "10", "0.200000", "0.030000", "hijkl"};
  rec_vector rec4{"0", "true", "3", "30", "6", "100", "0.300000", "0.040000", "mnopqr"};
  rec_vector rec5{"0", "false", "4", "40", "8", "1000", "0.400000", "0.050000", "stuvwx"};
  rec_vector rec6{"0", "true", "5", "50", "10", "10000", "0.500000", "0.060000", "yyy"};
  rec_vector rec7{"0", "false", "6", "60", "12", "100000", "0.600000", "0.070000", "zzz"};
  rec_vector rec8{"0", "true", "7", "70", "14", "1000000", "0.700000", "0.080000", "zzz"};

  ASSERT_EQ(mlog.record_size() * 0, mlog.append(rec1));
  ASSERT_EQ(mlog.record_size() * 1, mlog.append(rec2));
  ASSERT_EQ(mlog.record_size() * 2, mlog.append(rec3));
  ASSERT_EQ(mlog.record_size() * 3, mlog.append(rec4));
  ASSERT_EQ(mlog.record_size() * 4, mlog.append(rec5));
  ASSERT_EQ(mlog.record_size() * 5, mlog.append(rec6));
  ASSERT_EQ(mlog.record_size() * 6, mlog.append(rec7));
  ASSERT_EQ(mlog.record_size() * 7, mlog.append(rec8));

  rec_vector res1 = mlog.read(mlog.record_size() * 0);
  rec_vector res2 = mlog.read(mlog.record_size() * 1);
  rec_vector res3 = mlog.read(mlog.record_size() * 2);
  rec_vector res4 = mlog.read(mlog.record_size() * 3);
  rec_vector res5 = mlog.read(mlog.record_size() * 4);
  rec_vector res6 = mlog.read(mlog.record_size() * 5);
  rec_vector res7 = mlog.read(mlog.record_size() * 6);
  rec_vector res8 = mlog.read(mlog.record_size() * 7);

  ASSERT_EQ(rec1, res1);
  ASSERT_EQ(rec2, res2);
  ASSERT_EQ(rec3, res3);
  ASSERT_EQ(rec4, res4);
  ASSERT_EQ(rec5, res5);
  ASSERT_EQ(rec6, res6);
  ASSERT_EQ(rec7, res7);
  ASSERT_EQ(rec8, res8);
}

TEST_F(AtomicMultilogTest, ArchiveTest) {
  atomic_multilog mlog(
      "my_table",
      schema_builder().add_column(primitive_types::STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  test_append_and_get(mlog);
  mlog.archive();
}

TEST_F(AtomicMultilogTest, IndexTest) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  mlog.add_index("a");
  mlog.add_index("b");
  mlog.add_index("c", 10);
  mlog.add_index("d", 2);
  mlog.add_index("e", 100);
  mlog.add_index("f", 0.1);
  mlog.add_index("g", 0.01);
  mlog.add_index("h");

  mlog.append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog.append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog.append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog.append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog.append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog.append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog.append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog.append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  size_t i = 0;
  for (auto r = mlog.execute_filter("a == true"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("b > 4"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = mlog.execute_filter("c <= 30"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("d == 0"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = mlog.execute_filter("e <= 100"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("e >= 100"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() >= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog.execute_filter("f > 0.1"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = mlog.execute_filter("g < 0.06"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog.execute_filter("h == zzz"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(8).value().to_data().as<std::string>().substr(0, 3) == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.execute_filter("a == true && b > 4"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.execute_filter("a == true && (b > 4 || c <= 30)"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("a == true && (b > 4 || f > 0.1)"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);
}

TEST_F(AtomicMultilogTest, RemoveIndexTest) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);

  mlog.add_index("a", 1);
  mlog.add_index("b", 1);
  mlog.add_index("c", 10);
  mlog.add_index("d", 2);
  mlog.add_index("e", 100);
  mlog.add_index("f", 0.1);
  mlog.add_index("g", 0.01);
  mlog.add_index("h", 1);

  mlog.append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog.append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));

  try {
    mlog.remove_index("a");
    mlog.remove_index("a");
  } catch (std::exception &e) {
    std::string error_message = "Could not remove index for a: No index exists";
    ASSERT_STREQ(e.what(), error_message.c_str());
  }

  mlog.remove_index("b");
  ASSERT_EQ(false, mlog.is_indexed("b"));
  ASSERT_EQ(true, mlog.is_indexed("c"));
}

// TODO: Separate out the tests
// TODO: Add tests for aggregates only
TEST_F(AtomicMultilogTest, RemoveFilterTriggerTest) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  mlog.add_filter("filter1", "a == true");
  mlog.add_filter("filter2", "b > 4");
  mlog.add_aggregate("agg1", "filter1", "SUM(d)");
  mlog.add_aggregate("agg2", "filter2", "SUM(d)");
  mlog.install_trigger("trigger1", "agg1 >= 10");
  mlog.install_trigger("trigger2", "agg2 >= 10");

  int64_t now_ns = time_utils::cur_ns();
  uint64_t beg = now_ns / configuration_params::TIME_RESOLUTION_NS();
  uint64_t end = beg;
  mlog.append(record(now_ns, false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog.append(record(now_ns, true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog.append(record(now_ns, false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog.append(record(now_ns, true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog.append(record(now_ns, false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog.append(record(now_ns, true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog.append(record(now_ns, false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog.append(record(now_ns, true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  size_t i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end); r->has_more(); r->advance()) {
    i++;
  }

  try {
    mlog.remove_filter("filter1");
    mlog.query_filter("filter1", beg, end);
  } catch (std::exception &e) {
    std::string message = "Filter filter1 does not exist.";
    ASSERT_STREQ(e.what(), message.c_str());
  }

  try {
    mlog.remove_filter("filter2");
    mlog.remove_filter("filter2");
  } catch (std::exception &ex) {
    std::string message = "Filter filter2 does not exist.";
    ASSERT_STREQ(ex.what(), message.c_str());
  }

  size_t first_count = 0;
  for (auto a = mlog.get_alerts(beg, end); a->has_more(); a->advance()) {
    first_count++;
  }

  mlog.remove_trigger("trigger2");
  sleep(1);
  size_t second_count = 0;
  for (auto a = mlog.get_alerts(beg, end); a->has_more(); a->advance()) {
    second_count++;
  }

  ASSERT_LE(second_count, first_count);

  try {
    mlog.remove_trigger("trigger1");
    mlog.remove_trigger("trigger1");
  } catch (std::exception &e) {
    std::string message = "Trigger trigger1 does not exist.";
    ASSERT_STREQ(e.what(), message.c_str());
  }

}

// TODO: Separate out the tests
TEST_F(AtomicMultilogTest, FilterAggregateTriggerTest) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  mlog.add_filter("filter1", "a == true");
  mlog.add_filter("filter2", "b > 4");
  mlog.add_filter("filter3", "c <= 30");
  mlog.add_filter("filter4", "d == 0");
  mlog.add_filter("filter5", "e <= 100");
  mlog.add_filter("filter6", "f > 0.1");
  mlog.add_filter("filter7", "g < 0.06");
  mlog.add_filter("filter8", "h == zzz");
  mlog.add_aggregate("agg1", "filter1", "SUM(d)");
  mlog.add_aggregate("agg2", "filter2", "SUM(d)");
  mlog.add_aggregate("agg3", "filter3", "SUM(d)");
  mlog.add_aggregate("agg4", "filter4", "SUM(d)");
  mlog.add_aggregate("agg5", "filter5", "SUM(d)");
  mlog.add_aggregate("agg6", "filter6", "SUM(d)");
  mlog.add_aggregate("agg7", "filter7", "SUM(d)");
  mlog.add_aggregate("agg8", "filter8", "SUM(d)");
  mlog.install_trigger("trigger1", "agg1 >= 10");
  mlog.install_trigger("trigger2", "agg2 >= 10");
  mlog.install_trigger("trigger3", "agg3 >= 10");
  mlog.install_trigger("trigger4", "agg4 >= 10");
  mlog.install_trigger("trigger5", "agg5 >= 10");
  mlog.install_trigger("trigger6", "agg6 >= 10");
  mlog.install_trigger("trigger7", "agg7 >= 10");
  mlog.install_trigger("trigger8", "agg8 >= 10");

  int64_t now_ns = time_utils::cur_ns();
  uint64_t beg = now_ns / configuration_params::TIME_RESOLUTION_NS();
  uint64_t end = beg;
  mlog.append(record(now_ns, false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog.append(record(now_ns, true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog.append(record(now_ns, false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog.append(record(now_ns, true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog.append(record(now_ns, false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog.append(record(now_ns, true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog.append(record(now_ns, false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog.append(record(now_ns, true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  // Test filters
  size_t i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter2", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = mlog.query_filter("filter3", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter4", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = mlog.query_filter("filter5", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter6", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = mlog.query_filter("filter7", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog.query_filter("filter8", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(8).value().to_data().as<std::string>().substr(0, 3) == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end, "b > 4"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end, "b > 4 || c <= 30"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end, "b > 4 || f > 0.1"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test aggregates
  numeric val1 = mlog.get_aggregate("agg1", beg, end);
  ASSERT_TRUE(numeric(32) == val1);
  numeric val2 = mlog.get_aggregate("agg2", beg, end);
  ASSERT_TRUE(numeric(36) == val2);
  numeric val3 = mlog.get_aggregate("agg3", beg, end);
  ASSERT_TRUE(numeric(12) == val3);
  numeric val4 = mlog.get_aggregate("agg4", beg, end);
  ASSERT_TRUE(numeric(0) == val4);
  numeric val5 = mlog.get_aggregate("agg5", beg, end);
  ASSERT_TRUE(numeric(12) == val5);
  numeric val6 = mlog.get_aggregate("agg6", beg, end);
  ASSERT_TRUE(numeric(54) == val6);
  numeric val7 = mlog.get_aggregate("agg7", beg, end);
  ASSERT_TRUE(numeric(20) == val7);
  numeric val8 = mlog.get_aggregate("agg8", beg, end);
  ASSERT_TRUE(numeric(26) == val8);

  // Test triggers
  sleep(3);  // To make sure all triggers have been evaluated

  size_t alert_count = 0;
  for (auto a = mlog.get_alerts(beg, end); a->has_more(); a->advance()) {
    LOG_INFO << "Alert: " << a->get().to_string();
    ASSERT_TRUE(a->get().value >= numeric(10));
    alert_count++;
  }
  ASSERT_EQ(size_t(7), alert_count);

  auto a1 = mlog.get_alerts(beg, end, "trigger1");
  ASSERT_TRUE(a1->has_more());
  ASSERT_EQ("trigger1", a1->get().trigger_name);
  ASSERT_TRUE(numeric(32) == a1->get().value);
  a1->advance();
  ASSERT_TRUE(a1->empty());

  auto a2 = mlog.get_alerts(beg, end, "trigger2");
  ASSERT_TRUE(a2->has_more());
  ASSERT_EQ("trigger2", a2->get().trigger_name);
  ASSERT_TRUE(numeric(36) == a2->get().value);
  a2->advance();
  ASSERT_TRUE(a2->empty());

  auto a3 = mlog.get_alerts(beg, end, "trigger3");
  ASSERT_TRUE(a3->has_more());
  ASSERT_EQ("trigger3", a3->get().trigger_name);
  ASSERT_TRUE(numeric(12) == a3->get().value);
  a3->advance();
  ASSERT_TRUE(a3->empty());

  auto a4 = mlog.get_alerts(beg, end, "trigger4");
  ASSERT_TRUE(a4->empty());

  auto a5 = mlog.get_alerts(beg, end, "trigger5");
  ASSERT_TRUE(a5->has_more());
  ASSERT_EQ("trigger5", a5->get().trigger_name);
  ASSERT_TRUE(numeric(12) == a5->get().value);
  a5->advance();
  ASSERT_TRUE(a5->empty());

  auto a6 = mlog.get_alerts(beg, end, "trigger6");
  ASSERT_TRUE(a6->has_more());
  ASSERT_EQ("trigger6", a6->get().trigger_name);
  ASSERT_TRUE(numeric(54) == a6->get().value);
  a6->advance();
  ASSERT_TRUE(a6->empty());

  auto a7 = mlog.get_alerts(beg, end, "trigger7");
  ASSERT_TRUE(a7->has_more());
  ASSERT_EQ("trigger7", a7->get().trigger_name);
  ASSERT_TRUE(numeric(20) == a7->get().value);
  a7->advance();
  ASSERT_TRUE(a7->empty());

  auto a8 = mlog.get_alerts(beg, end, "trigger8");
  ASSERT_TRUE(a8->has_more());
  ASSERT_EQ("trigger8", a8->get().trigger_name);
  ASSERT_TRUE(numeric(26) == a8->get().value);
  a8->advance();
  ASSERT_TRUE(a8->empty());
}

TEST_F(AtomicMultilogTest, BatchIndexTest) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  mlog.add_index("a");
  mlog.add_index("b");
  mlog.add_index("c", 10);
  mlog.add_index("d", 2);
  mlog.add_index("e", 100);
  mlog.add_index("f", 0.1);
  mlog.add_index("g", 0.01);
  mlog.add_index("h");

  record_batch batch = build_batch(mlog);

  mlog.append_batch(batch);

  size_t i = 0;
  for (auto r = mlog.execute_filter("a == true"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("b > 4"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = mlog.execute_filter("c <= 30"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("d == 0"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = mlog.execute_filter("e <= 100"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("e >= 100"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() >= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog.execute_filter("f > 0.1"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = mlog.execute_filter("g < 0.06"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog.execute_filter("h == zzz"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(8).value().to_data().as<std::string>().substr(0, 3) == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.execute_filter("a == true && b > 4"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.execute_filter("a == true && (b > 4 || c <= 30)"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.execute_filter("a == true && (b > 4 || f > 0.1)"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);
}

// TODO: Separate out the tests
// TODO: Add tests for aggregates only
TEST_F(AtomicMultilogTest, BatchFilterAggregateTriggerTest) {
  atomic_multilog mlog("my_table", s, "/tmp", storage::IN_MEMORY, archival_mode::OFF, MGMT_POOL);
  mlog.add_filter("filter1", "a == true");
  mlog.add_filter("filter2", "b > 4");
  mlog.add_filter("filter3", "c <= 30");
  mlog.add_filter("filter4", "d == 0");
  mlog.add_filter("filter5", "e <= 100");
  mlog.add_filter("filter6", "f > 0.1");
  mlog.add_filter("filter7", "g < 0.06");
  mlog.add_filter("filter8", "h == zzz");
  mlog.add_aggregate("agg1", "filter1", "SUM(d)");
  mlog.add_aggregate("agg2", "filter2", "SUM(d)");
  mlog.add_aggregate("agg3", "filter3", "SUM(d)");
  mlog.add_aggregate("agg4", "filter4", "SUM(d)");
  mlog.add_aggregate("agg5", "filter5", "SUM(d)");
  mlog.add_aggregate("agg6", "filter6", "SUM(d)");
  mlog.add_aggregate("agg7", "filter7", "SUM(d)");
  mlog.add_aggregate("agg8", "filter8", "SUM(d)");
  mlog.install_trigger("trigger1", "agg1 >= 10");
  mlog.install_trigger("trigger2", "agg2 >= 10");
  mlog.install_trigger("trigger3", "agg3 >= 10");
  mlog.install_trigger("trigger4", "agg4 >= 10");
  mlog.install_trigger("trigger5", "agg5 >= 10");
  mlog.install_trigger("trigger6", "agg6 >= 10");
  mlog.install_trigger("trigger7", "agg7 >= 10");
  mlog.install_trigger("trigger8", "agg8 >= 10");

  int64_t now_ns = time_utils::cur_ns();
  int64_t beg = now_ns / configuration_params::TIME_RESOLUTION_NS();
  int64_t end = beg;
  record_batch batch = build_batch(mlog, now_ns);

  mlog.append_batch(batch);

  // Test filters
  size_t i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter2", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = mlog.query_filter("filter3", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter4", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = mlog.query_filter("filter5", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter6", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = mlog.query_filter("filter7", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog.query_filter("filter8", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(
        r->get().at(8).value().to_data().as<std::string>().substr(0, 3) == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end, "b > 4"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end, "b > 4 || c <= 30"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog.query_filter("filter1", beg, end, "b > 4 || f > 0.1"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test aggregates
  numeric val1 = mlog.get_aggregate("agg1", beg, end);
  ASSERT_TRUE(numeric(32) == val1);
  numeric val2 = mlog.get_aggregate("agg2", beg, end);
  ASSERT_TRUE(numeric(36) == val2);
  numeric val3 = mlog.get_aggregate("agg3", beg, end);
  ASSERT_TRUE(numeric(12) == val3);
  numeric val4 = mlog.get_aggregate("agg4", beg, end);
  ASSERT_TRUE(numeric(0) == val4);
  numeric val5 = mlog.get_aggregate("agg5", beg, end);
  ASSERT_TRUE(numeric(12) == val5);
  numeric val6 = mlog.get_aggregate("agg6", beg, end);
  ASSERT_TRUE(numeric(54) == val6);
  numeric val7 = mlog.get_aggregate("agg7", beg, end);
  ASSERT_TRUE(numeric(20) == val7);
  numeric val8 = mlog.get_aggregate("agg8", beg, end);
  ASSERT_TRUE(numeric(26) == val8);

  // Test triggers
  sleep(1);  // To make sure all triggers have been evaluated

  size_t alert_count = 0;
  for (auto a = mlog.get_alerts(beg, end); a->has_more(); a->advance()) {
    LOG_INFO << "Alert: " << a->get().to_string();
    ASSERT_TRUE(a->get().value >= numeric(10));
    alert_count++;
  }
  ASSERT_EQ(size_t(7), alert_count);

  auto a1 = mlog.get_alerts(beg, end, "trigger1");
  ASSERT_TRUE(a1->has_more());
  ASSERT_EQ("trigger1", a1->get().trigger_name);
  ASSERT_TRUE(numeric(32) == a1->get().value);
  a1->advance();
  ASSERT_TRUE(a1->empty());

  auto a2 = mlog.get_alerts(beg, end, "trigger2");
  ASSERT_TRUE(a2->has_more());
  ASSERT_EQ("trigger2", a2->get().trigger_name);
  ASSERT_TRUE(numeric(36) == a2->get().value);
  a2->advance();
  ASSERT_TRUE(a2->empty());

  auto a3 = mlog.get_alerts(beg, end, "trigger3");
  ASSERT_TRUE(a3->has_more());
  ASSERT_EQ("trigger3", a3->get().trigger_name);
  ASSERT_TRUE(numeric(12) == a3->get().value);
  a3->advance();
  ASSERT_TRUE(a3->empty());

  auto a4 = mlog.get_alerts(beg, end, "trigger4");
  ASSERT_TRUE(a4->empty());

  auto a5 = mlog.get_alerts(beg, end, "trigger5");
  ASSERT_TRUE(a5->has_more());
  ASSERT_EQ("trigger5", a5->get().trigger_name);
  ASSERT_TRUE(numeric(12) == a5->get().value);
  a5->advance();
  ASSERT_TRUE(a5->empty());

  auto a6 = mlog.get_alerts(beg, end, "trigger6");
  ASSERT_TRUE(a6->has_more());
  ASSERT_EQ("trigger6", a6->get().trigger_name);
  ASSERT_TRUE(numeric(54) == a6->get().value);
  a6->advance();
  ASSERT_TRUE(a6->empty());

  auto a7 = mlog.get_alerts(beg, end, "trigger7");
  ASSERT_TRUE(a7->has_more());
  ASSERT_EQ("trigger7", a7->get().trigger_name);
  ASSERT_TRUE(numeric(20) == a7->get().value);
  a7->advance();
  ASSERT_TRUE(a7->empty());

  auto a8 = mlog.get_alerts(beg, end, "trigger8");
  ASSERT_TRUE(a8->has_more());
  ASSERT_EQ("trigger8", a8->get().trigger_name);
  ASSERT_TRUE(numeric(26) == a8->get().value);
  a8->advance();
  ASSERT_TRUE(a8->empty());
}

#endif /* CONFLUO_TEST_ATOMIC_MULTILOG_TEST_H_ */

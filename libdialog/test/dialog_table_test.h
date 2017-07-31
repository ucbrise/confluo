#ifndef TEST_DIALOG_TABLE_TEST_H_
#define TEST_DIALOG_TABLE_TEST_H_

#include "dialog_table.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::dialog;

class DiaLogTableTest : public testing::Test {
 public:
  static task_pool MGMT_POOL;
  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_append_and_get(dialog_table& dtable) {
    std::vector<uint64_t> offsets;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      DiaLogTableTest::generate_bytes(data_, DATA_SIZE, i);
      uint64_t offset = dtable.append(data_);
      offsets.push_back(offset);
    }

    record_t r;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      bool success = dtable.read(offsets[i], r);
      const uint8_t* ret = reinterpret_cast<const uint8_t*>(r.data());
      ASSERT_TRUE(success);
      uint8_t expected = i % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ret[j], expected);
      }
    }
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

  void* record(bool a, int8_t b, int16_t c, int32_t d, int64_t e, float f,
               double g, const char* h) {
    int64_t ts = utils::time_utils::cur_ns();
    r = {ts, a, b, c, d, e, f, g, {}};
    memcpy(r.h, h, std::min(static_cast<size_t>(16), strlen(h)));
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

 protected:
  uint8_t data_[DATA_SIZE];

  virtual void SetUp() override {
    thread_manager::register_thread();
  }

  virtual void TearDown() override {
    thread_manager::deregister_thread();
  }
};

DiaLogTableTest::rec DiaLogTableTest::r;
std::vector<column_t> DiaLogTableTest::s = schema();
task_pool DiaLogTableTest::MGMT_POOL;

TEST_F(DiaLogTableTest, AppendAndGetInMemoryTest) {
  dialog_table dtable(
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::IN_MEMORY, MGMT_POOL);
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, AppendAndGetDurableTest) {
  dialog_table dtable(
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::DURABLE, MGMT_POOL);
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, AppendAndGetDurableRelaxedTest) {
  dialog_table dtable(
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      "/tmp", storage::DURABLE_RELAXED, MGMT_POOL);
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, IndexTest) {
  dialog_table dtable(s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  dtable.add_index("a");
  dtable.add_index("b");
  dtable.add_index("c", 10);
  dtable.add_index("d", 2);
  dtable.add_index("e", 100);
  dtable.add_index("f", 0.1);
  dtable.add_index("g", 0.01);
  dtable.add_index("h");

  dtable.append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  dtable.append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  dtable.append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  dtable.append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  dtable.append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  dtable.append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  dtable.append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  dtable.append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  size_t i = 0;
  for (auto r = dtable.execute_filter("a == true"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.execute_filter("b > 4"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = dtable.execute_filter("c <= 30"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.execute_filter("d == 0"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = dtable.execute_filter("e <= 100"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.execute_filter("f > 0.1"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = dtable.execute_filter("g < 0.06"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = dtable.execute_filter("h == zzz"); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable.execute_filter("a == true && b > 4"); r.has_more();
      ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable.execute_filter("a == true && (b > 4 || c <= 30)");
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.execute_filter("a == true && (b > 4 || f > 0.1)");
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);
}

TEST_F(DiaLogTableTest, FilterTest) {
  dialog_table dtable(s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  dtable.add_filter("filter1", "a == true");
  dtable.add_filter("filter2", "b > 4");
  dtable.add_filter("filter3", "c <= 30");
  dtable.add_filter("filter4", "d == 0");
  dtable.add_filter("filter5", "e <= 100");
  dtable.add_filter("filter6", "f > 0.1");
  dtable.add_filter("filter7", "g < 0.06");
  dtable.add_filter("filter8", "h == zzz");
  dtable.add_trigger("trigger1", "filter1", "SUM(d) >= 10");
  dtable.add_trigger("trigger2", "filter2", "SUM(d) >= 10");
  dtable.add_trigger("trigger3", "filter3", "SUM(d) >= 10");
  dtable.add_trigger("trigger4", "filter4", "SUM(d) >= 10");
  dtable.add_trigger("trigger5", "filter5", "SUM(d) >= 10");
  dtable.add_trigger("trigger6", "filter6", "SUM(d) >= 10");
  dtable.add_trigger("trigger7", "filter7", "SUM(d) >= 10");
  dtable.add_trigger("trigger8", "filter8", "SUM(d) >= 10");

  dtable.append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  int64_t beg = filter::get_ts_block(r.ts);
  dtable.append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  dtable.append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  dtable.append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  dtable.append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  dtable.append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  dtable.append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  dtable.append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
  int64_t end = filter::get_ts_block(r.ts);

  size_t i = 0;
  for (auto r = dtable.query_filter("filter1", beg, end); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.query_filter("filter2", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = dtable.query_filter("filter3", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.query_filter("filter4", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = dtable.query_filter("filter5", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.query_filter("filter6", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = dtable.query_filter("filter7", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = dtable.query_filter("filter8", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable.query_filter("filter1", "b > 4", beg, end); r.has_more();
      ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable.query_filter("filter1", "b > 4 || c <= 30", beg, end);
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable.query_filter("filter1", "b > 4 || f > 0.1", beg, end);
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test triggers
  sleep(1);  // To make sure all triggers have been evaluated

  const std::set<alert>& alerts = dtable.get_alerts();
  if (!alerts.empty()) {
    ASSERT_TRUE(static_cast<size_t>(7) <= alerts.size());
    std::set<std::string> trigger_names;
    for (const auto& a : alerts) {
      trigger_names.insert(a.trig->trigger_name());
    }

    ASSERT_EQ(static_cast<size_t>(7), trigger_names.size());
  }
}

#endif /* TEST_DIALOG_TABLE_TEST_H_ */

#ifndef TEST_READER_TEST_H_
#define TEST_READER_TEST_H_

#include <iostream>
#include "dialog_server.h"
#include "dialog_table.h"
#include "gtest/gtest.h"

#include "rpc_dialog_reader.h"
#include "rpc_dialog_writer.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::dialog::rpc;
using namespace ::dialog;

class ReaderTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;

  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_read(dialog_table* dtable, rpc_dialog_reader& client) {
    std::vector<int64_t> offsets;
    for (int64_t i = 0; i < MAX_RECORDS; i++) {
      generate_bytes(data_, DATA_SIZE, i);
      int64_t offset = dtable->append(data_);
      offsets.push_back(offset);
    }

    std::string buf;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      client.read(buf, offsets[i]);
      ASSERT_EQ(dtable->record_size(), buf.size());
      uint8_t* data = reinterpret_cast<uint8_t*>(&buf[0]);
      ASSERT_TRUE(data != nullptr);
      uint8_t expected = i % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(expected, data[j]);
      }
    }
    ASSERT_EQ(MAX_RECORDS, client.num_records());
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

  static dialog_store* simple_table_store(const std::string& table_name,
                                          storage::storage_id id) {
    auto store = new dialog_store("/tmp");
    store->add_table(
        table_name,
        schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
        id);
    return store;
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

ReaderTest::rec ReaderTest::r;
std::vector<column_t> ReaderTest::s = schema();

TEST_F(ReaderTest, ReadInMemoryTest) {
  std::string table_name = "my_table";
  auto store = simple_table_store(table_name, storage::D_IN_MEMORY);
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client(SERVER_ADDRESS, SERVER_PORT, table_name);
  test_read(store->get_table(table_name), client);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}

TEST_F(ReaderTest, ReadDurableTest) {
  std::string table_name = "my_table";
  auto store = simple_table_store(table_name, storage::D_DURABLE);
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client(SERVER_ADDRESS, SERVER_PORT, table_name);
  test_read(store->get_table(table_name), client);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}

TEST_F(ReaderTest, ReadDurableRelaxedTest) {
  std::string table_name = "my_table";
  auto store = simple_table_store(table_name, storage::D_DURABLE_RELAXED);
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client(SERVER_ADDRESS, SERVER_PORT, table_name);
  test_read(store->get_table(table_name), client);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}

TEST_F(ReaderTest, AdHocFilterTest) {
  std::string table_name = "my_table";
  auto store = new dialog_store("/tmp");
  store->add_table(table_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);

  dtable->add_index("a");
  dtable->add_index("b");
  dtable->add_index("c", 10);
  dtable->add_index("d", 2);
  dtable->add_index("e", 100);
  dtable->add_index("f", 0.1);
  dtable->add_index("g", 0.01);
  dtable->add_index("h");

  dtable->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  dtable->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  dtable->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  dtable->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  dtable->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  dtable->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  dtable->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  dtable->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client(SERVER_ADDRESS, SERVER_PORT, table_name);

  size_t i = 0;
  for (auto r = client.adhoc_filter("a == true"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("b > 4"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.adhoc_filter("c <= 30"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("d == 0"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.adhoc_filter("e <= 100"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("f > 0.1"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.adhoc_filter("g < 0.06"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.adhoc_filter("h == zzz"); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.adhoc_filter("a == true && b > 4"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.adhoc_filter("a == true && (b > 4 || c <= 30)");
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("a == true && (b > 4 || f > 0.1)");
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ReaderTest, PreDefFilterTest) {
  std::string table_name = "my_table";
  auto store = new dialog_store("/tmp");
  store->add_table(table_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);

  dtable->add_filter("filter1", "a == true");
  dtable->add_filter("filter2", "b > 4");
  dtable->add_filter("filter3", "c <= 30");
  dtable->add_filter("filter4", "d == 0");
  dtable->add_filter("filter5", "e <= 100");
  dtable->add_filter("filter6", "f > 0.1");
  dtable->add_filter("filter7", "g < 0.06");
  dtable->add_filter("filter8", "h == zzz");
  dtable->add_trigger("trigger1", "filter1", "SUM(d) >= 10");
  dtable->add_trigger("trigger2", "filter2", "SUM(d) >= 10");
  dtable->add_trigger("trigger3", "filter3", "SUM(d) >= 10");
  dtable->add_trigger("trigger4", "filter4", "SUM(d) >= 10");
  dtable->add_trigger("trigger5", "filter5", "SUM(d) >= 10");
  dtable->add_trigger("trigger6", "filter6", "SUM(d) >= 10");
  dtable->add_trigger("trigger7", "filter7", "SUM(d) >= 10");
  dtable->add_trigger("trigger8", "filter8", "SUM(d) >= 10");

  dtable->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  int64_t beg = filter::get_ts_block(r.ts);
  dtable->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  dtable->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  dtable->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  dtable->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  dtable->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  dtable->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  dtable->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
  int64_t end = filter::get_ts_block(r.ts);

  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client = rpc_dialog_reader(SERVER_ADDRESS, SERVER_PORT,
                                               table_name);
  size_t i = 0;
  for (auto r = client.predef_filter("filter1", beg, end); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.predef_filter("filter2", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.predef_filter("filter3", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.predef_filter("filter4", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.predef_filter("filter5", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.predef_filter("filter6", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.predef_filter("filter7", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.predef_filter("filter8", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.combined_filter("filter1", "b > 4", beg, end);
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.combined_filter("filter1", "b > 4 || c <= 30", beg, end);
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.combined_filter("filter1", "b > 4 || f > 0.1", beg, end);
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

  std::set<std::string> trigger_names;
  for (auto alerts = client.get_alerts(beg, end); alerts.has_more(); ++alerts) {
    LOG_INFO<< "Alert: " << alerts.get();
    trigger_names.insert(alerts.get());
  }
  ASSERT_EQ(static_cast<size_t>(7), trigger_names.size());

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}
TEST_F(ReaderTest, BatchAdHocFilterTest) {

  std::string table_name = "my_table";
  auto store = new dialog_store("/tmp");
  store->add_table(table_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);

  dtable->add_index("a");
  dtable->add_index("b");
  dtable->add_index("c", 10);
  dtable->add_index("d", 2);
  dtable->add_index("e", 100);
  dtable->add_index("f", 0.1);
  dtable->add_index("g", 0.01);
  dtable->add_index("h");

  record_batch batch = get_batch();
  dtable->append_batch(batch);

  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client(SERVER_ADDRESS, SERVER_PORT, table_name);

  size_t i = 0;
  for (auto r = client.adhoc_filter("a == true"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("b > 4"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.adhoc_filter("c <= 30"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("d == 0"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.adhoc_filter("e <= 100"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("f > 0.1"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.adhoc_filter("g < 0.06"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.adhoc_filter("h == zzz"); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.adhoc_filter("a == true && b > 4"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.adhoc_filter("a == true && (b > 4 || c <= 30)");
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.adhoc_filter("a == true && (b > 4 || f > 0.1)");
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ReaderTest, BatchPreDefFilterTest) {

  std::string table_name = "my_table";
  auto store = new dialog_store("/tmp");
  store->add_table(table_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);

  dtable->add_filter("filter1", "a == true");
  dtable->add_filter("filter2", "b > 4");
  dtable->add_filter("filter3", "c <= 30");
  dtable->add_filter("filter4", "d == 0");
  dtable->add_filter("filter5", "e <= 100");
  dtable->add_filter("filter6", "f > 0.1");
  dtable->add_filter("filter7", "g < 0.06");
  dtable->add_filter("filter8", "h == zzz");
  dtable->add_trigger("trigger1", "filter1", "SUM(d) >= 10");
  dtable->add_trigger("trigger2", "filter2", "SUM(d) >= 10");
  dtable->add_trigger("trigger3", "filter3", "SUM(d) >= 10");
  dtable->add_trigger("trigger4", "filter4", "SUM(d) >= 10");
  dtable->add_trigger("trigger5", "filter5", "SUM(d) >= 10");
  dtable->add_trigger("trigger6", "filter6", "SUM(d) >= 10");
  dtable->add_trigger("trigger7", "filter7", "SUM(d) >= 10");
  dtable->add_trigger("trigger8", "filter8", "SUM(d) >= 10");

  record_batch batch = get_batch();
  dtable->append_batch(batch);

  int64_t beg = batch.start_time_block();
  int64_t end = batch.end_time_block();

  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_reader client(SERVER_ADDRESS, SERVER_PORT, table_name);

  size_t i = 0;
  for (auto r = client.predef_filter("filter1", beg, end); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.predef_filter("filter2", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.predef_filter("filter3", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.predef_filter("filter4", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.predef_filter("filter5", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.predef_filter("filter6", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.predef_filter("filter7", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.predef_filter("filter8", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.combined_filter("filter1", "b > 4", beg, end);
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.combined_filter("filter1", "b > 4 || c <= 30", beg, end);
      r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.combined_filter("filter1", "b > 4 || f > 0.1", beg, end);
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

  std::set<std::string> trigger_names;
  for (auto alerts = client.get_alerts(beg, end); alerts.has_more(); ++alerts) {
    LOG_INFO<< "Alert: " << alerts.get();
    trigger_names.insert(alerts.get());
  }
  ASSERT_EQ(static_cast<size_t>(7), trigger_names.size());

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* TEST_READER_TEST_H_ */

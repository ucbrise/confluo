#ifndef RPC_TEST_READ_OPS_TEST_H_
#define RPC_TEST_READ_OPS_TEST_H_

#include <iostream>
#include "gtest/gtest.h"

#include "atomic_multilog.h"
#include "rpc_client.h"
#include "rpc_server.h"
#include "rpc_test_utils.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo::rpc;
using namespace ::confluo;

class ClientReadOpsTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;

  static void generate_bytes(uint8_t *buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_read(atomic_multilog *mlog, rpc_client &client) {
    std::vector<int64_t> offsets;
    for (int64_t i = 0; i < MAX_RECORDS; i++) {
      generate_bytes(data_, DATA_SIZE, i);
      int64_t offset = mlog->append(data_);
      offsets.push_back(offset);
    }

    record_data buf;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      client.read(buf, offsets[i]);
      ASSERT_EQ(mlog->record_size(), buf.size());
      uint8_t *data = reinterpret_cast<uint8_t *>(&buf[0]);
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

  static void *record(bool a, int8_t b, int16_t c, int32_t d, int64_t e,
                      float f, double g, const char *h) {
    int64_t ts = utils::time_utils::cur_ns();
    r = {ts, a, b, c, d, e, f, g, {}};
    size_t len = std::min(static_cast<size_t>(16), strlen(h));
    memcpy(r.h, h, len);
    for (size_t i = len; i < 16; i++) {
      r.h[i] = '\0';
    }
    return reinterpret_cast<void *>(&r);
  }

  static void *record(int64_t ts, bool a, int8_t b, int16_t c, int32_t d,
                      int64_t e, float f, double g, const char *h) {
    r = {ts, a, b, c, d, e, f, g, {}};
    size_t len = std::min(static_cast<size_t>(16), strlen(h));
    memcpy(r.h, h, len);
    for (size_t i = len; i < 16; i++) {
      r.h[i] = '\0';
    }
    return reinterpret_cast<void *>(&r);
  }

  static confluo_store *simple_table_store(const std::string &multilog_name,
                                           storage::storage_mode id) {
    auto store = new confluo_store("/tmp");
    store->create_atomic_multilog(
        multilog_name,
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

  static record_batch build_batch(const atomic_multilog &mlog) {
    record_batch_builder builder = mlog.get_batch_builder();
    builder.add_record(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    builder.add_record(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
    builder.add_record(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
    builder.add_record(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
    builder.add_record(
        record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
    builder.add_record(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
    builder.add_record(
        record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
    builder.add_record(
        record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
    return builder.get_batch();
  }

  static record_batch build_batch(const atomic_multilog &mlog, int64_t ts) {
    record_batch_builder builder = mlog.get_batch_builder();
    builder.add_record(record(ts, false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    builder.add_record(record(ts, true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
    builder.add_record(record(ts, false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
    builder.add_record(record(ts, true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
    builder.add_record(
        record(ts, false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
    builder.add_record(record(ts, true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
    builder.add_record(
        record(ts, false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
    builder.add_record(
        record(ts, true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
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

ClientReadOpsTest::rec ClientReadOpsTest::r;
std::vector<column_t> ClientReadOpsTest::s = schema();

TEST_F(ClientReadOpsTest, ReadInMemoryTest) {
  std::string multilog_name = "my_multilog";
  auto store = simple_table_store(multilog_name, storage::IN_MEMORY);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  test_read(store->get_atomic_multilog(multilog_name), client);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}

TEST_F(ClientReadOpsTest, ReadDurableTest) {
  std::string multilog_name = "my_multilog";
  auto store = simple_table_store(multilog_name, storage::DURABLE);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  test_read(store->get_atomic_multilog(multilog_name), client);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}

TEST_F(ClientReadOpsTest, ReadDurableRelaxedTest) {
  std::string multilog_name = "my_multilog";
  auto store = simple_table_store(multilog_name, storage::DURABLE_RELAXED);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  test_read(store->get_atomic_multilog(multilog_name), client);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}

TEST_F(ClientReadOpsTest, AdHocFilterTest) {
  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(multilog_name);

  mlog->add_index("a");
  mlog->add_index("b");
  mlog->add_index("c", 10);
  mlog->add_index("d", 2);
  mlog->add_index("e", 100);
  mlog->add_index("f", 0.1);
  mlog->add_index("g", 0.01);
  mlog->add_index("h");

  mlog->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  size_t i = 0;
  for (auto r = client.execute_filter("a == true"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("b > 4"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.execute_filter("c <= 30"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("d == 0"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.execute_filter("e <= 100"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("f > 0.1"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.execute_filter("g < 0.06"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.execute_filter("h == zzz"); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.execute_filter("a == true && b > 4"); r.has_more();
       ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.execute_filter("a == true && (b > 4 || c <= 30)");
       r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("a == true && (b > 4 || f > 0.1)");
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

TEST_F(ClientReadOpsTest, FilterAggregateTriggerTest) {
  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(multilog_name);

  mlog->add_filter("filter1", "a == true");
  mlog->add_filter("filter2", "b > 4");
  mlog->add_filter("filter3", "c <= 30");
  mlog->add_filter("filter4", "d == 0");
  mlog->add_filter("filter5", "e <= 100");
  mlog->add_filter("filter6", "f > 0.1");
  mlog->add_filter("filter7", "g < 0.06");
  mlog->add_filter("filter8", "h == zzz");
  mlog->add_aggregate("agg1", "filter1", "SUM(d)");
  mlog->add_aggregate("agg2", "filter2", "SUM(d)");
  mlog->add_aggregate("agg3", "filter3", "SUM(d)");
  mlog->add_aggregate("agg4", "filter4", "SUM(d)");
  mlog->add_aggregate("agg5", "filter5", "SUM(d)");
  mlog->add_aggregate("agg6", "filter6", "SUM(d)");
  mlog->add_aggregate("agg7", "filter7", "SUM(d)");
  mlog->add_aggregate("agg8", "filter8", "SUM(d)");
  mlog->install_trigger("trigger1", "agg1 >= 10");
  mlog->install_trigger("trigger2", "agg2 >= 10");
  mlog->install_trigger("trigger3", "agg3 >= 10");
  mlog->install_trigger("trigger4", "agg4 >= 10");
  mlog->install_trigger("trigger5", "agg5 >= 10");
  mlog->install_trigger("trigger6", "agg6 >= 10");
  mlog->install_trigger("trigger7", "agg7 >= 10");
  mlog->install_trigger("trigger8", "agg8 >= 10");

  int64_t now_ns = time_utils::cur_ns();
  int64_t beg = now_ns / configuration_params::TIME_RESOLUTION_NS;
  int64_t end = beg;
  mlog->append(record(now_ns, false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog->append(record(now_ns, true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog->append(record(now_ns, false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog->append(record(now_ns, true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog->append(record(now_ns, false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog->append(record(now_ns, true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog->append(record(now_ns, false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog->append(record(now_ns, true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client = rpc_client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  // Test filters
  size_t i = 0;
  for (auto r = client.query_filter("filter1", beg, end); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter2", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.query_filter("filter3", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter4", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.query_filter("filter5", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter6", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.query_filter("filter7", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.query_filter("filter8", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.query_filter("filter1", beg, end, "b > 4"); r.has_more();
       ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.query_filter("filter1", beg, end, "b > 4 || c <= 30");
       r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter1", beg, end, "b > 4 || f > 0.1");
       r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test aggregates
  std::string val1 = client.get_aggregate("agg1", beg, end);
  fprintf(stderr, "Aggregate: %s\n", val1.c_str());
  ASSERT_TRUE("double(32.000000)" == val1);
  std::string val2 = client.get_aggregate("agg2", beg, end);
  ASSERT_TRUE("double(36.000000)" == val2);
  std::string val3 = client.get_aggregate("agg3", beg, end);
  ASSERT_TRUE("double(12.000000)" == val3);
  std::string val4 = client.get_aggregate("agg4", beg, end);
  ASSERT_TRUE("double(0.000000)" == val4);
  std::string val5 = client.get_aggregate("agg5", beg, end);
  ASSERT_TRUE("double(12.000000)" == val5);
  std::string val6 = client.get_aggregate("agg6", beg, end);
  ASSERT_TRUE("double(54.000000)" == val6);
  std::string val7 = client.get_aggregate("agg7", beg, end);
  ASSERT_TRUE("double(20.000000)" == val7);
  std::string val8 = client.get_aggregate("agg8", beg, end);
  ASSERT_TRUE("double(26.000000)" == val8);

  // Test triggers
  sleep(1);  // To make sure all triggers have been evaluated

  size_t alert_count = 0;
  for (auto alerts = client.get_alerts(beg, end); alerts.has_more(); ++alerts) {
    LOG_INFO << "Alert: " << alerts.get();
    alert_count++;
  }
  ASSERT_EQ(size_t(7), alert_count);

  // TODO: more rigorous testing on alert values.
  auto a1 = client.get_alerts(beg, end, "trigger1");
  ASSERT_TRUE(!a1.empty());

  auto a2 = client.get_alerts(beg, end, "trigger2");
  ASSERT_TRUE(!a2.empty());

  auto a3 = client.get_alerts(beg, end, "trigger3");
  ASSERT_TRUE(!a3.empty());

  auto a4 = client.get_alerts(beg, end, "trigger4");
  ASSERT_TRUE(a4.empty());

  auto a5 = client.get_alerts(beg, end, "trigger5");
  ASSERT_TRUE(!a5.empty());

  auto a6 = client.get_alerts(beg, end, "trigger6");
  ASSERT_TRUE(!a6.empty());

  auto a7 = client.get_alerts(beg, end, "trigger7");
  ASSERT_TRUE(!a7.empty());

  auto a8 = client.get_alerts(beg, end, "trigger8");
  ASSERT_TRUE(!a8.empty());

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }

}
TEST_F(ClientReadOpsTest, BatchAdHocFilterTest) {

  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(multilog_name);

  mlog->add_index("a");
  mlog->add_index("b");
  mlog->add_index("c", 10);
  mlog->add_index("d", 2);
  mlog->add_index("e", 100);
  mlog->add_index("f", 0.1);
  mlog->add_index("g", 0.01);
  mlog->add_index("h");

  record_batch batch = build_batch(*mlog);
  mlog->append_batch(batch);

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  size_t i = 0;
  for (auto r = client.execute_filter("a == true"); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("b > 4"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.execute_filter("c <= 30"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("d == 0"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.execute_filter("e <= 100"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("f > 0.1"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.execute_filter("g < 0.06"); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.execute_filter("h == zzz"); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.execute_filter("a == true && b > 4"); r.has_more();
       ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.execute_filter("a == true && (b > 4 || c <= 30)");
       r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.execute_filter("a == true && (b > 4 || f > 0.1)");
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

TEST_F(ClientReadOpsTest, BatchFilterAggregateTriggerTest) {

  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(multilog_name);

  mlog->add_filter("filter1", "a == true");
  mlog->add_filter("filter2", "b > 4");
  mlog->add_filter("filter3", "c <= 30");
  mlog->add_filter("filter4", "d == 0");
  mlog->add_filter("filter5", "e <= 100");
  mlog->add_filter("filter6", "f > 0.1");
  mlog->add_filter("filter7", "g < 0.06");
  mlog->add_filter("filter8", "h == zzz");
  mlog->add_aggregate("agg1", "filter1", "SUM(d)");
  mlog->add_aggregate("agg2", "filter2", "SUM(d)");
  mlog->add_aggregate("agg3", "filter3", "SUM(d)");
  mlog->add_aggregate("agg4", "filter4", "SUM(d)");
  mlog->add_aggregate("agg5", "filter5", "SUM(d)");
  mlog->add_aggregate("agg6", "filter6", "SUM(d)");
  mlog->add_aggregate("agg7", "filter7", "SUM(d)");
  mlog->add_aggregate("agg8", "filter8", "SUM(d)");
  mlog->install_trigger("trigger1", "agg1 >= 10");
  mlog->install_trigger("trigger2", "agg2 >= 10");
  mlog->install_trigger("trigger3", "agg3 >= 10");
  mlog->install_trigger("trigger4", "agg4 >= 10");
  mlog->install_trigger("trigger5", "agg5 >= 10");
  mlog->install_trigger("trigger6", "agg6 >= 10");
  mlog->install_trigger("trigger7", "agg7 >= 10");
  mlog->install_trigger("trigger8", "agg8 >= 10");

  int64_t now_ns = time_utils::cur_ns();
  int64_t beg = now_ns / configuration_params::TIME_RESOLUTION_NS;
  int64_t end = beg;
  record_batch batch = build_batch(*mlog, now_ns);
  mlog->append_batch(batch);

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(multilog_name);

  // Test filters
  size_t i = 0;
  for (auto r = client.query_filter("filter1", beg, end); r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter2", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = client.query_filter("filter3", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter4", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = client.query_filter("filter5", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter6", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = client.query_filter("filter7", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(r.get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = client.query_filter("filter8", beg, end); r.has_more(); ++r) {
    ASSERT_TRUE(
        r.get().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.query_filter("filter1", beg, end, "b > 4"); r.has_more();
       ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = client.query_filter("filter1", beg, end, "b > 4 || c <= 30");
       r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = client.query_filter("filter1", beg, end, "b > 4 || f > 0.1");
       r.has_more(); ++r) {
    ASSERT_EQ(true, r.get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.get().at(2).value().to_data().as<int8_t>() > '4'
            || r.get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test aggregates
  std::string val1 = client.get_aggregate("agg1", beg, end);
  ASSERT_TRUE("double(32.000000)" == val1);
  std::string val2 = client.get_aggregate("agg2", beg, end);
  ASSERT_TRUE("double(36.000000)" == val2);
  std::string val3 = client.get_aggregate("agg3", beg, end);
  ASSERT_TRUE("double(12.000000)" == val3);
  std::string val4 = client.get_aggregate("agg4", beg, end);
  ASSERT_TRUE("double(0.000000)" == val4);
  std::string val5 = client.get_aggregate("agg5", beg, end);
  ASSERT_TRUE("double(12.000000)" == val5);
  std::string val6 = client.get_aggregate("agg6", beg, end);
  ASSERT_TRUE("double(54.000000)" == val6);
  std::string val7 = client.get_aggregate("agg7", beg, end);
  ASSERT_TRUE("double(20.000000)" == val7);
  std::string val8 = client.get_aggregate("agg8", beg, end);
  ASSERT_TRUE("double(26.000000)" == val8);

  // Test triggers
  sleep(1);  // To make sure all triggers have been evaluated

  size_t alert_count = 0;
  for (auto alerts = client.get_alerts(beg, end); alerts.has_more(); ++alerts) {
    LOG_INFO << "Alert: " << alerts.get();
    alert_count++;
  }
  ASSERT_EQ(size_t(7), alert_count);

  // TODO: more rigorous testing on alert values.
  auto a1 = client.get_alerts(beg, end, "trigger1");
  ASSERT_TRUE(!a1.empty());

  auto a2 = client.get_alerts(beg, end, "trigger2");
  ASSERT_TRUE(!a2.empty());

  auto a3 = client.get_alerts(beg, end, "trigger3");
  ASSERT_TRUE(!a3.empty());

  auto a4 = client.get_alerts(beg, end, "trigger4");
  ASSERT_TRUE(a4.empty());

  auto a5 = client.get_alerts(beg, end, "trigger5");
  ASSERT_TRUE(!a5.empty());

  auto a6 = client.get_alerts(beg, end, "trigger6");
  ASSERT_TRUE(!a6.empty());

  auto a7 = client.get_alerts(beg, end, "trigger7");
  ASSERT_TRUE(!a7.empty());

  auto a8 = client.get_alerts(beg, end, "trigger8");
  ASSERT_TRUE(!a8.empty());

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* RPC_TEST_READ_OPS_TEST_H_ */

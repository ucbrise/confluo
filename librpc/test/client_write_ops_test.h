#ifndef RPC_TEST_WRITE_OPS_TEST_H_
#define RPC_TEST_WRITE_OPS_TEST_H_

#include "gtest/gtest.h"

#include "atomic_multilog.h"
#include "rpc_client.h"
#include "rpc_server.h"
#include "rpc_test_utils.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo::rpc;
using namespace ::confluo;

class ClientWriteOpsTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;

  static void generate_bytes(uint8_t *buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
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

  static std::string record_str(bool a, int8_t b, int16_t c, int32_t d,
                                int64_t e, float f, double g, const char *h) {
    void *rbuf = record(a, b, c, d, e, f, g, h);
    return std::string(reinterpret_cast<const char *>(rbuf), sizeof(rec));
  }

  static record_data make_simple_record(int64_t ts, const std::string &str) {
    record_data data;
    data.resize(sizeof(int64_t) + DATA_SIZE);
    size_t len = std::min(str.length(), static_cast<size_t>(DATA_SIZE));
    memcpy(&data[0], &ts, sizeof(int64_t));
    memcpy(&data[0] + sizeof(int64_t), str.data(), len);
    memset(&data[0] + sizeof(int64_t) + len, '\0',
           sizeof(int64_t) + DATA_SIZE - len);
    return data;
  }

  static confluo_store *simple_multilog_store(std::string atomic_multilog_name,
                                              storage::storage_mode mode) {
    auto store = new confluo_store("/tmp");
    store->create_atomic_multilog(
        atomic_multilog_name,
        schema_builder().add_column(primitive_types::STRING_TYPE(DATA_SIZE), "msg").get_columns(),
        mode);
    return store;
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

 protected:
  uint8_t data_[DATA_SIZE];

  virtual void SetUp() override {
    thread_manager::register_thread();
  }

  virtual void TearDown() override {
    thread_manager::deregister_thread();
  }
};

ClientWriteOpsTest::rec ClientWriteOpsTest::r;
std::vector<column_t> ClientWriteOpsTest::s = schema();

TEST_F(ClientWriteOpsTest, CreateTableTest) {

  std::string atomic_multilog_name = "my_multilog";

  auto store = new confluo_store("/tmp");
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);

  client.create_atomic_multilog(
      atomic_multilog_name,
      schema_t(schema_builder().add_column(primitive_types::STRING_TYPE(DATA_SIZE), "msg").get_columns()),
      storage::IN_MEMORY);

  client.disconnect();
  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, WriteTest) {
  std::string atomic_multilog_name = "my_multilog";

  auto store = simple_multilog_store(atomic_multilog_name, storage::storage_mode::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(atomic_multilog_name);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(atomic_multilog_name);

  int64_t ts = utils::time_utils::cur_ns();
  client.append(make_simple_record(ts, "abc"));

  std::string buf = mlog->read(0)[1];
  ASSERT_EQ(buf, "abc");

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, AddIndexTest) {

  std::string atomic_multilog_name = "my_multilog";

  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(atomic_multilog_name, schema(),
                                storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(atomic_multilog_name);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(atomic_multilog_name);

  client.add_index("a", 1);
  client.add_index("b", 1);
  client.add_index("c", 10);
  client.add_index("d", 2);
  client.add_index("e", 100);
  client.add_index("f", 0.1);
  client.add_index("g", 0.01);
  client.add_index("h", 1);

  mlog->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  size_t i = 0;
  for (auto r = mlog->execute_filter("a == true"); r->has_more();
       r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->execute_filter("b > 4"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = mlog->execute_filter("c <= 30"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->execute_filter("d == 0"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = mlog->execute_filter("e <= 100"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->execute_filter("f > 0.1"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = mlog->execute_filter("g < 0.06"); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog->execute_filter("h == zzz"); r->has_more(); r->advance()) {
    ASSERT_TRUE(
        r->get().at(8).value().to_data().as<std::string>().substr(0, 3) == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog->execute_filter("a == true && b > 4"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog->execute_filter("a == true && (b > 4 || c <= 30)"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->execute_filter("a == true && (b > 4 || f > 0.1)"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, AddFilterAndTriggerTest) {

  std::string atomic_multilog_name = "my_multilog";

  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(atomic_multilog_name, schema(), storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(atomic_multilog_name);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(atomic_multilog_name);
  client.add_filter("filter1", "a == true");
  client.add_filter("filter2", "b > 4");
  client.add_filter("filter3", "c <= 30");
  client.add_filter("filter4", "d == 0");
  client.add_filter("filter5", "e <= 100");
  client.add_filter("filter6", "f > 0.1");
  client.add_filter("filter7", "g < 0.06");
  client.add_filter("filter8", "h == zzz");
  client.add_aggregate("agg1", "filter1", "SUM(d)");
  client.add_aggregate("agg2", "filter2", "SUM(d)");
  client.add_aggregate("agg3", "filter3", "SUM(d)");
  client.add_aggregate("agg4", "filter4", "SUM(d)");
  client.add_aggregate("agg5", "filter5", "SUM(d)");
  client.add_aggregate("agg6", "filter6", "SUM(d)");
  client.add_aggregate("agg7", "filter7", "SUM(d)");
  client.add_aggregate("agg8", "filter8", "SUM(d)");
  client.install_trigger("trigger1", "agg1 >= 10");
  client.install_trigger("trigger2", "agg2 >= 10");
  client.install_trigger("trigger3", "agg3 >= 10");
  client.install_trigger("trigger4", "agg4 >= 10");
  client.install_trigger("trigger5", "agg5 >= 10");
  client.install_trigger("trigger6", "agg6 >= 10");
  client.install_trigger("trigger7", "agg7 >= 10");
  client.install_trigger("trigger8", "agg8 >= 10");

  int64_t now_ns = time_utils::cur_ns();
  uint64_t beg = now_ns / configuration_params::TIME_RESOLUTION_NS();
  uint64_t end = beg;
  mlog->append(record(now_ns, false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog->append(record(now_ns, true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog->append(record(now_ns, false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog->append(record(now_ns, true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog->append(record(now_ns, false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog->append(record(now_ns, true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog->append(record(now_ns, false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog->append(record(now_ns, true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  size_t i = 0;
  for (auto r = mlog->query_filter("filter1", beg, end); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->query_filter("filter2", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = mlog->query_filter("filter3", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->query_filter("filter4", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = mlog->query_filter("filter5", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->query_filter("filter6", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = mlog->query_filter("filter7", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = mlog->query_filter("filter8", beg, end); r->has_more(); r->advance()) {
    ASSERT_TRUE(r->get().at(8).value().to_data().as<std::string>().substr(0, 3) == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog->query_filter("filter1", beg, end, "b > 4"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r->get().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = mlog->query_filter("filter1", beg, end, "b > 4 || c <= 30"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = mlog->query_filter("filter1", beg, end, "b > 4 || f > 0.1"); r->has_more(); r->advance()) {
    ASSERT_EQ(true, r->get().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r->get().at(2).value().to_data().as<int8_t>() > '4' || r->get().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test aggregates
  numeric val1 = mlog->get_aggregate("agg1", beg, end);
  ASSERT_TRUE(numeric(32) == val1);
  numeric val2 = mlog->get_aggregate("agg2", beg, end);
  ASSERT_TRUE(numeric(36) == val2);
  numeric val3 = mlog->get_aggregate("agg3", beg, end);
  ASSERT_TRUE(numeric(12) == val3);
  numeric val4 = mlog->get_aggregate("agg4", beg, end);
  ASSERT_TRUE(numeric(0) == val4);
  numeric val5 = mlog->get_aggregate("agg5", beg, end);
  ASSERT_TRUE(numeric(12) == val5);
  numeric val6 = mlog->get_aggregate("agg6", beg, end);
  ASSERT_TRUE(numeric(54) == val6);
  numeric val7 = mlog->get_aggregate("agg7", beg, end);
  ASSERT_TRUE(numeric(20) == val7);
  numeric val8 = mlog->get_aggregate("agg8", beg, end);
  ASSERT_TRUE(numeric(26) == val8);

  // Test triggers
  sleep(1);  // To make sure all triggers have been evaluated

  size_t alert_count = 0;
  for (auto a = mlog->get_alerts(beg, end); a->has_more(); a->advance()) {
    LOG_INFO << "Alert: " << a->get().to_string();
    ASSERT_TRUE(a->get().value >= numeric(10));
    alert_count++;
  }
  ASSERT_EQ(size_t(7), alert_count);

  auto a1 = mlog->get_alerts(beg, end, "trigger1");
  ASSERT_TRUE(a1->has_more());
  ASSERT_EQ("trigger1", a1->get().trigger_name);
  ASSERT_TRUE(numeric(32) == a1->get().value);
  a1->advance();
  ASSERT_TRUE(a1->empty());

  auto a2 = mlog->get_alerts(beg, end, "trigger2");
  ASSERT_TRUE(a2->has_more());
  ASSERT_EQ("trigger2", a2->get().trigger_name);
  ASSERT_TRUE(numeric(36) == a2->get().value);
  a2->advance();
  ASSERT_TRUE(a2->empty());

  auto a3 = mlog->get_alerts(beg, end, "trigger3");
  ASSERT_TRUE(a3->has_more());
  ASSERT_EQ("trigger3", a3->get().trigger_name);
  ASSERT_TRUE(numeric(12) == a3->get().value);
  a3->advance();
  ASSERT_TRUE(a3->empty());

  auto a4 = mlog->get_alerts(beg, end, "trigger4");
  ASSERT_TRUE(a4->empty());

  auto a5 = mlog->get_alerts(beg, end, "trigger5");
  ASSERT_TRUE(a5->has_more());
  ASSERT_EQ("trigger5", a5->get().trigger_name);
  ASSERT_TRUE(numeric(12) == a5->get().value);
  a5->advance();
  ASSERT_TRUE(a5->empty());

  auto a6 = mlog->get_alerts(beg, end, "trigger6");
  ASSERT_TRUE(a6->has_more());
  ASSERT_EQ("trigger6", a6->get().trigger_name);
  ASSERT_TRUE(numeric(54) == a6->get().value);
  a6->advance();
  ASSERT_TRUE(a6->empty());

  auto a7 = mlog->get_alerts(beg, end, "trigger7");
  ASSERT_TRUE(a7->has_more());
  ASSERT_EQ("trigger7", a7->get().trigger_name);
  ASSERT_TRUE(numeric(20) == a7->get().value);
  a7->advance();
  ASSERT_TRUE(a7->empty());

  auto a8 = mlog->get_alerts(beg, end, "trigger8");
  ASSERT_TRUE(a8->has_more());
  ASSERT_EQ("trigger8", a8->get().trigger_name);
  ASSERT_TRUE(numeric(26) == a8->get().value);
  a8->advance();
  ASSERT_TRUE(a8->empty());

  client.disconnect();
  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, RemoveIndexTest) {
  std::string atomic_multilog_name = "my_multilog";

  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(atomic_multilog_name, schema(),
                                storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(atomic_multilog_name);

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });
  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(atomic_multilog_name);

  client.add_index("a", 1);
  client.add_index("b", 1);
  client.add_index("c", 10);
  client.add_index("d", 2);
  client.add_index("e", 100);
  client.add_index("f", 0.1);
  client.add_index("g", 0.01);
  client.add_index("h", 1);

  mlog->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));

  try {
    client.remove_index("a");
    client.remove_index("a");
  } catch (std::exception &e) {
    std::string error_message = "TException - service has thrown: "
                                "rpc_management_exception(msg=Could not remove index for a:"
                                " No index exists)";
    ASSERT_STREQ(e.what(), error_message.c_str());
  }

  client.remove_index("b");
  ASSERT_EQ(false, mlog->is_indexed("b"));
  ASSERT_EQ(true, mlog->is_indexed("c"));

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, RemoveFilterTriggerTest) {
  std::string atomic_multilog_name = "my_multilog";

  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(atomic_multilog_name, schema(),
                                storage::IN_MEMORY);
  auto mlog = store->get_atomic_multilog(atomic_multilog_name);
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  rpc_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_atomic_multilog(atomic_multilog_name);

  client.add_filter("filter1", "a == true");
  client.add_filter("filter2", "b > 4");
  client.add_aggregate("agg1", "filter1", "SUM(d)");
  client.add_aggregate("agg2", "filter2", "SUM(d)");
  client.install_trigger("trigger1", "agg1 >= 10");
  client.install_trigger("trigger2", "agg2 >= 10");

  uint64_t beg = r.ts / configuration_params::TIME_RESOLUTION_NS();
  mlog->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  mlog->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  mlog->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  mlog->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  mlog->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  mlog->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  mlog->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  mlog->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  uint64_t end = r.ts / configuration_params::TIME_RESOLUTION_NS();

  size_t i = 0;
  for (auto r = mlog->query_filter("filter1", beg, end); r->has_more();
       r->advance()) {
    i++;
  }

  try {
    client.remove_filter("filter1");
    mlog->query_filter("filter1", beg, end);
  } catch (std::exception &e) {
    std::string message = "Filter filter1 does not exist.";
    ASSERT_STREQ(e.what(), message.c_str());
  }

  try {
    client.remove_filter("filter2");
    client.remove_filter("filter2");
  } catch (std::exception &ex) {
    std::string message = "TException - service has thrown: "
                          "rpc_management_exception(msg=Filter filter2 does not "
                          "exist.)";
    ASSERT_STREQ(ex.what(), message.c_str());
  }

  size_t first_count = 0;
  for (auto a = mlog->get_alerts(static_cast<uint64_t>(beg), static_cast<uint64_t>(end)); a->has_more(); a->advance()) {
    first_count++;
  }

  client.remove_trigger("trigger2");
  sleep(1);
  size_t second_count = 0;
  for (auto a = mlog->get_alerts(static_cast<uint64_t>(beg), static_cast<uint64_t>(end)); a->has_more(); a->advance()) {
    second_count++;
  }

  ASSERT_LE(second_count, first_count);

  try {
    client.remove_trigger("trigger1");
    client.remove_trigger("trigger1");
  } catch (std::exception &e) {
    std::string message = "TException - service has thrown: "
                          "rpc_management_exception(msg=Trigger trigger1 does not "
                          "exist.)";
    ASSERT_STREQ(e.what(), message.c_str());
  }

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* RPC_TEST_WRITE_OPS_TEST_H_ */

#ifndef TEST_WRITER_TEST_H_
#define TEST_WRITER_TEST_H_

#include "dialog_server.h"
#include "dialog_table.h"
#include "gtest/gtest.h"

#include "rpc_dialog_client.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo::rpc;
using namespace ::confluo;

class ClientWriteOpsTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;

  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
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

  static std::string pad_str(std::string str, size_t size) {
    str.insert(str.end(), size - str.length(), '\0');
    return str;
  }

  static dialog_store* simple_table_store(std::string table_name,
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

// TODO: test rpc_dialog_client remove functions
TEST_F(ClientWriteOpsTest, RemoveIndexTest) {
    std::string table_name = "my_table";

    auto store = new dialog_store("/tmp");
    store->add_table(table_name, schema(), storage::D_IN_MEMORY);
    auto dtable = store->get_table(table_name);
    
    auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
    std::thread serve_thread([&server] {
        server->serve();
    });
    sleep(1);

    rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
    client.set_current_table(table_name);

    client.add_index("a", 1);
    client.add_index("b", 1);
    client.add_index("c", 10);
    client.add_index("d", 2);
    client.add_index("e", 100);
    client.add_index("f", 0.1);
    client.add_index("g", 0.01);
    client.add_index("h", 1);

    dtable->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    dtable->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));

    try {
        client.remove_index("a");
        client.remove_index("a");
    } catch (std::exception& e) {
        std::string error_message = "TException - service has thrown: " \
            "rpc_management_exception(msg=Could not remove index for a:" \
            " No index exists)";
        ASSERT_STREQ(e.what(), error_message.c_str());
    }

    client.remove_index("b");
    ASSERT_EQ(false, dtable->is_indexed("b"));
    ASSERT_EQ(true, dtable->is_indexed("c"));

    client.disconnect();
    server->stop();
    if (serve_thread.joinable()) {
        serve_thread.join();
    }
}

TEST_F(ClientWriteOpsTest, RemoveFilterTriggerTest) {
    std::string table_name = "my_table";

    auto store = new dialog_store("/tmp");
    store->add_table(table_name, schema(), storage::D_IN_MEMORY);
    auto dtable = store->get_table(table_name);
    auto server = dialog_server::create(store, SERVER_ADDRESS, 
            SERVER_PORT);
    std::thread serve_thread([&server] {
        server->serve();
    });

    sleep(1);

    rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
    client.set_current_table(table_name);

    client.add_filter("filter1", "a == true");
    client.add_filter("filter2", "b > 4");
    
    client.add_trigger("trigger1", "filter2", "SUM(d) >= 10");
    client.add_trigger("trigger2", "filter2", "SUM(d) >= 10");
    
    int64_t beg = r.ts / configuration_params::TIME_RESOLUTION_NS;
    dtable->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
    dtable->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
    dtable->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
    dtable->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
    dtable->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
    dtable->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
    dtable->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
    dtable->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

    int64_t end = r.ts / configuration_params::TIME_RESOLUTION_NS;

    size_t i = 0;
    for (auto r = dtable->query_filter("filter1", beg, end); 
            !r.empty(); r = r.tail()) {
        i++;
    }

    try {
        client.remove_filter("filter1");
        dtable->query_filter("filter1", beg, end);
    } catch (std::exception& e) {
        std::string message = "Filter filter1 does not exist.";
        ASSERT_STREQ(e.what(), message.c_str());
    }


    try {
        client.remove_filter("filter2");
        client.remove_filter("filter2");
    } catch (std::exception& ex) {
        std::string message = "TException - service has thrown: " \
            "rpc_management_exception(msg=Filter filter2 does not " \
            "exist.)";
        ASSERT_STREQ(ex.what(), message.c_str());
    }

    auto before_alerts = dtable->get_alerts(beg, end);
    client.remove_trigger("trigger2");
    sleep(1);
    auto after_alerts = dtable->get_alerts(beg, end);
    
    size_t first_count = 0;
    size_t second_count = 0;

    for (const auto& a : before_alerts) {
        first_count++;
    }

    for (const auto& a : after_alerts) {
        second_count++;
    }
    
    ASSERT_LE(second_count, first_count);

    try {
        client.remove_trigger("trigger1");
        client.remove_trigger("trigger1");
    } catch (std::exception& e) {
        std::string message = "TException - service has thrown: " \
            "rpc_management_exception(msg=Trigger trigger1 does not " \
            "exist.)";
        ASSERT_STREQ(e.what(), message.c_str());
    }
  
    client.disconnect();
    server->stop();
    if (serve_thread.joinable()) {
        serve_thread.join();
    }
}

TEST_F(ClientWriteOpsTest, CreateTableTest) {

  std::string table_name = "my_table";

  auto store = new dialog_store("/tmp");
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);

  client.create_table(
      table_name,
      schema_t(
          schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns()),
      storage::D_IN_MEMORY);

  client.disconnect();
  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, WriteTest) {

  std::string table_name = "my_table";

  auto store = simple_table_store(table_name, storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_table(table_name);

  int64_t ts = utils::time_utils::cur_ns();
  std::string ts_str = std::string(reinterpret_cast<const char*>(&ts), 8);
  client.write(ts_str + pad_str("abc", DATA_SIZE));

  ro_data_ptr ptr;
  dtable->read(0, ptr);
  std::string buf = std::string(reinterpret_cast<const char*>(ptr.get()),
  DATA_SIZE);
  ASSERT_EQ(buf.substr(8, 3), "abc");

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, BufferTest) {

  std::string table_name = "my_table";

  auto store = simple_table_store(table_name, storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);
  auto schema_size = dtable->get_schema().record_size();
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_table(table_name);

  int64_t ts = utils::time_utils::cur_ns();
  std::string ts_str = std::string(reinterpret_cast<const char*>(&ts), 8);
  client.buffer(ts_str + pad_str("abc", DATA_SIZE));
  client.buffer(ts_str + pad_str("def", DATA_SIZE));
  client.buffer(ts_str + pad_str("ghi", DATA_SIZE));
  client.flush();

  ro_data_ptr ptr;

  dtable->read(0, ptr);
  std::string buf = std::string(reinterpret_cast<const char*>(ptr.get()), DATA_SIZE);
  ASSERT_EQ(buf.substr(8, 3), "abc");

  dtable->read(schema_size, ptr);
  std::string buf2 = std::string(reinterpret_cast<const char*>(ptr.get()), DATA_SIZE);
  ASSERT_EQ(buf2.substr(8, 3), "def");

  dtable->read(schema_size * 2, ptr);
  std::string buf3 = std::string(reinterpret_cast<const char*>(ptr.get()), DATA_SIZE);
  ASSERT_EQ(buf3.substr(8, 3), "ghi");

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(ClientWriteOpsTest, AddIndexTest) {

  std::string table_name = "my_table";

  auto store = new dialog_store("/tmp");
  store->add_table(table_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_table(table_name);

  client.add_index("a", 1);
  client.add_index("b", 1);
  client.add_index("c", 10);
  client.add_index("d", 2);
  client.add_index("e", 100);
  client.add_index("f", 0.1);
  client.add_index("g", 0.01);
  client.add_index("h", 1);

  dtable->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  dtable->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  dtable->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  dtable->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  dtable->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  dtable->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  dtable->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  dtable->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));

  size_t i = 0;
  for (auto r = dtable->execute_filter("a == true"); !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->execute_filter("b > 4"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = dtable->execute_filter("c <= 30"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->execute_filter("d == 0"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = dtable->execute_filter("e <= 100"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->execute_filter("f > 0.1"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = dtable->execute_filter("g < 0.06"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = dtable->execute_filter("h == zzz"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(
        r.head().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable->execute_filter("a == true && b > 4"); !r.empty();
      r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.head().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable->execute_filter("a == true && (b > 4 || c <= 30)");
      !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.head().at(2).value().to_data().as<int8_t>() > '4'
            || r.head().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->execute_filter("a == true && (b > 4 || f > 0.1)");
      !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.head().at(2).value().to_data().as<int8_t>() > '4'
            || r.head().at(6).value().to_data().as<float>() > 0.1);
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

  std::string table_name = "my_table";

  auto store = new dialog_store("/tmp");
  store->add_table(table_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_table(table_name);
  auto server = dialog_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
  client.set_current_table(table_name);

  client.add_filter("filter1", "a == true");
  client.add_filter("filter2", "b > 4");
  client.add_filter("filter3", "c <= 30");
  client.add_filter("filter4", "d == 0");
  client.add_filter("filter5", "e <= 100");
  client.add_filter("filter6", "f > 0.1");
  client.add_filter("filter7", "g < 0.06");
  client.add_filter("filter8", "h == zzz");
  client.add_trigger("trigger1", "filter1", "SUM(d) >= 10");
  client.add_trigger("trigger2", "filter2", "SUM(d) >= 10");
  client.add_trigger("trigger3", "filter3", "SUM(d) >= 10");
  client.add_trigger("trigger4", "filter4", "SUM(d) >= 10");
  client.add_trigger("trigger5", "filter5", "SUM(d) >= 10");
  client.add_trigger("trigger6", "filter6", "SUM(d) >= 10");
  client.add_trigger("trigger7", "filter7", "SUM(d) >= 10");
  client.add_trigger("trigger8", "filter8", "SUM(d) >= 10");

  dtable->append(record(false, '0', 0, 0, 0, 0.0, 0.01, "abc"));
  int64_t beg = r.ts / configuration_params::TIME_RESOLUTION_NS;
  dtable->append(record(true, '1', 10, 2, 1, 0.1, 0.02, "defg"));
  dtable->append(record(false, '2', 20, 4, 10, 0.2, 0.03, "hijkl"));
  dtable->append(record(true, '3', 30, 6, 100, 0.3, 0.04, "mnopqr"));
  dtable->append(record(false, '4', 40, 8, 1000, 0.4, 0.05, "stuvwx"));
  dtable->append(record(true, '5', 50, 10, 10000, 0.5, 0.06, "yyy"));
  dtable->append(record(false, '6', 60, 12, 100000, 0.6, 0.07, "zzz"));
  dtable->append(record(true, '7', 70, 14, 1000000, 0.7, 0.08, "zzz"));
  int64_t end = r.ts / configuration_params::TIME_RESOLUTION_NS;

  size_t i = 0;
  for (auto r = dtable->query_filter("filter1", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->query_filter("filter2", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  i = 0;
  for (auto r = dtable->query_filter("filter3", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->query_filter("filter4", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(4).value().to_data().as<int32_t>() == 0);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(1), i);

  i = 0;
  for (auto r = dtable->query_filter("filter5", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(5).value().to_data().as<int64_t>() <= 100);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->query_filter("filter6", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(6), i);

  i = 0;
  for (auto r = dtable->query_filter("filter7", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(r.head().at(7).value().to_data().as<double>() < 0.06);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(5), i);

  i = 0;
  for (auto r = dtable->query_filter("filter8", beg, end); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(
        r.head().at(8).value().to_data().as<std::string>().substr(0, 3)
            == "zzz");
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable->query_filter("filter1", "b > 4", beg, end);
      !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(r.head().at(2).value().to_data().as<int8_t>() > '4');
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(2), i);

  i = 0;
  for (auto r = dtable->query_filter("filter1", "b > 4 || c <= 30", beg, end);
      !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.head().at(2).value().to_data().as<int8_t>() > '4'
            || r.head().at(3).value().to_data().as<int16_t>() <= 30);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(4), i);

  i = 0;
  for (auto r = dtable->query_filter("filter1", "b > 4 || f > 0.1", beg, end);
      !r.empty(); r = r.tail()) {
    ASSERT_EQ(true, r.head().at(1).value().to_data().as<bool>());
    ASSERT_TRUE(
        r.head().at(2).value().to_data().as<int8_t>() > '4'
            || r.head().at(6).value().to_data().as<float>() > 0.1);
    i++;
  }
  ASSERT_EQ(static_cast<size_t>(3), i);

  // Test triggers
  sleep(1);  // To make sure all triggers have been evaluated

  auto alerts = dtable->get_alerts(beg, end);
  for (const auto& a : alerts) {
    LOG_INFO<< "Alert: " << a.to_string();
  }

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

#endif /* TEST_WRITER_TEST_H_ */

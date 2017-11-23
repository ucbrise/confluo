#ifndef EXAMPLES_TEST_STREAM_TEST_H_
#define EXAMPLES_TEST_STREAM_TEST_H_

#include "stream_consumer.h"
#include "stream_producer.h"
#include "atomic_multilog.h"

#include "rpc_client.h"
#include "rpc_server.h"

#include "math.h"
#include "gtest/gtest.h"

#include "streaming_test_utils.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;
using namespace ::confluo::rpc;

class StreamTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;

  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  void test_read(atomic_multilog* dtable, rpc_client& client) {
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

  static confluo_store* simple_table_store(const std::string& multilog_name,
                                           storage::storage_id id) {
    auto store = new confluo_store("/tmp");
    store->create_atomic_multilog(
        multilog_name,
        schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
        id);
    return store;
  }

  static std::string pad_str(std::string str, size_t size) {
    str.insert(str.end(), size - str.length(), '\0');
    return str;
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

TEST_F(StreamTest, WriteTest) {
  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_atomic_multilog(multilog_name);

  dtable->add_index("a");
  dtable->add_index("b");
  dtable->add_index("c", 1);
  dtable->add_index("d", 2);
  dtable->add_index("e", 100);
  dtable->add_index("f", 0.1);
  dtable->add_index("g", 0.01);
  dtable->add_index("h");

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });
  streaming_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  uint64_t buffer_timeout = static_cast<uint64_t>(1e30);
  stream_producer sp(SERVER_ADDRESS, SERVER_PORT, 20, buffer_timeout);
  sp.set_current_atomic_multilog(multilog_name);

  std::vector<std::string> expected_strings;
  uint64_t k_max = 10000;
  for (uint64_t i = 0; i < k_max; i++) {
    std::string record_string = record_str(true, '7', i, 14, 1000, 0.7, 0.02,
                                           "stream");
    sp.buffer(record_string);
    expected_strings.push_back(record_string);
  }
  sp.flush();

  std::string buf;
  for (uint64_t i = 0; i < k_max; i++) {
    sp.read(buf, i * sizeof(rec));
    ASSERT_EQ(dtable->record_size(), buf.size());
    ASSERT_STREQ(expected_strings[i].c_str(), buf.c_str());
  }

  sp.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(StreamTest, BufferTest) {

  std::string multilog_name = "my_multilog";

  auto store = simple_table_store(multilog_name, storage::D_IN_MEMORY);
  auto dtable = store->get_atomic_multilog(multilog_name);
  auto schema_size = dtable->get_schema().record_size();
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  streaming_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  uint64_t buffer_timeout = static_cast<uint64_t>(1e30);
  stream_producer client(SERVER_ADDRESS, SERVER_PORT, 20, buffer_timeout);
  client.set_current_atomic_multilog(multilog_name);

  int64_t ts = utils::time_utils::cur_ns();
  std::string ts_str = std::string(reinterpret_cast<const char*>(&ts), 8);
  client.buffer(ts_str + pad_str("abc", DATA_SIZE));
  client.buffer(ts_str + pad_str("def", DATA_SIZE));
  client.buffer(ts_str + pad_str("ghi", DATA_SIZE));
  client.flush();

  ro_data_ptr ptr;

  dtable->read(0, ptr);
  std::string buf = std::string(reinterpret_cast<const char*>(ptr.get()),
  DATA_SIZE);
  ASSERT_EQ(buf.substr(8, 3), "abc");

  dtable->read(schema_size, ptr);
  std::string buf2 = std::string(reinterpret_cast<const char*>(ptr.get()),
  DATA_SIZE);
  ASSERT_EQ(buf2.substr(8, 3), "def");

  dtable->read(schema_size * 2, ptr);
  std::string buf3 = std::string(reinterpret_cast<const char*>(ptr.get()),
  DATA_SIZE);
  ASSERT_EQ(buf3.substr(8, 3), "ghi");

  client.disconnect();
  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(StreamTest, ReadTest) {
  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_atomic_multilog(multilog_name);

  dtable->add_index("a");
  dtable->add_index("b");
  dtable->add_index("c", 1);
  dtable->add_index("d", 2);
  dtable->add_index("e", 100);
  dtable->add_index("f", 0.1);
  dtable->add_index("g", 0.01);
  dtable->add_index("h");

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  streaming_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  stream_consumer sc(SERVER_ADDRESS, SERVER_PORT, 128);
  sc.set_current_atomic_multilog(multilog_name);

  std::vector<std::string> expected_strings;
  uint64_t k_max = 10000;

  for (uint64_t i = 0; i < k_max; i++) {
    std::string record_string = record_str(true, '7', i, 14, 1000, 0.7, 0.02,
                                           "stream");
    sc.write(record_string);
    expected_strings.push_back(record_string);
  }

  std::string data;
  for (uint64_t i = 0; i < k_max; i++) {
    sc.consume(data);
    ASSERT_EQ(dtable->record_size(), data.size());
    ASSERT_STREQ(expected_strings[i].c_str(), data.c_str());
  }

  sc.disconnect();

  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(StreamTest, ReadWriteTest) {
  std::string multilog_name = "my_multilog";
  auto store = new confluo_store("/tmp");
  store->create_atomic_multilog(multilog_name, schema(), storage::D_IN_MEMORY);
  auto dtable = store->get_atomic_multilog(multilog_name);

  dtable->add_index("a");
  dtable->add_index("b");
  dtable->add_index("c", 1);
  dtable->add_index("d", 2);
  dtable->add_index("e", 100);
  dtable->add_index("f", 0.1);
  dtable->add_index("g", 0.01);
  dtable->add_index("h");

  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  streaming_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);

  uint64_t buffer_timeout = static_cast<uint64_t>(1e30);
  stream_producer sp(SERVER_ADDRESS, SERVER_PORT, 20, buffer_timeout);
  sp.set_current_atomic_multilog(multilog_name);

  std::vector<std::string> expected_strings;
  uint64_t k_max = 10000;
  for (uint64_t i = 0; i < k_max; i++) {
    std::string record_string = record_str(true, '7', i, 14, 1000, 0.7, 0.02,
                                           "stream");
    sp.buffer(record_string);
    expected_strings.push_back(record_string);
  }

  stream_consumer sc(SERVER_ADDRESS, SERVER_PORT, 128);
  sc.set_current_atomic_multilog(multilog_name);

  std::string data;
  for (uint64_t i = 0; i < k_max; i++) {
    sc.consume(data);
    ASSERT_EQ(dtable->record_size(), data.size());
    ASSERT_STREQ(expected_strings[i].c_str(), data.c_str());
  }

  sc.disconnect();
  sp.disconnect();

  server->stop();
  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* EXAMPLES_TEST_STREAM_TEST_H_ */

#ifndef TEST_WRITER_TEST_H_
#define TEST_WRITER_TEST_H_

#include "dialog_server.h"
#include "dialog_table.h"
#include "gtest/gtest.h"

#include "../rpc/rpc_dialog_writer.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::dialog::rpc;
using namespace ::dialog;

class WriterTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;
  static task_pool MGMT_POOL;

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

  static dialog_store* single_table_store(std::string table_name, storage::storage_id id) {
    auto store = new dialog_store("/tmp");
    store->add_table(
      table_name,
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      id);
    return store;
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

 protected:
  uint8_t data_[DATA_SIZE];

  virtual void SetUp() override {
    thread_manager::register_thread();
  }

  virtual void TearDown() override {
    thread_manager::deregister_thread();
  }
};

TEST_F(WriterTest, CreateTableTest) {

  std::string table_name = "my_table";
  std::string server_address = "127.0.0.1";
  int server_port = 9090;

  auto store = new dialog_store("/tmp");
  auto server = dialog_server::create(store, "127.0.0.1", 9090);
  std::thread serve_thread([&server] {
    server->serve();
  });

  auto client = rpc_dialog_writer();
  client.connect("127.0.0.1", 9090);

//  client.create_table(
//      table_name,
//      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
//      storage::D_IN_MEMORY);

  client.disconnect();
  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

TEST_F(WriterTest, CreateTableTest) {

  std::string table_name = "my_table";
  std::string server_address = "127.0.0.1";
  int server_port = 9090;

  client.disconnect();
  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* TEST_WRITER_TEST_H_ */

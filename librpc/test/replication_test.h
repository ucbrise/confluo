#ifndef TEST_REPLICATION_TEST_H_
#define TEST_REPLICATION_TEST_H_

#include "dialog_server.h"
#include "dialog_table.h"
#include "gtest/gtest.h"

#include "rpc_endpoint.h"
#include "rpc_dialog_client.h"

using namespace ::dialog::rpc;
using namespace ::dialog;

#define DATA_SIZE   64U

class ReplicationTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 11001;
  const rpc_endpoint SERVER_EP = rpc_endpoint("127.0.0.1:11001");
  const rpc_endpoint REPLICA1_EP = rpc_endpoint("127.0.0.1:11002");
  const rpc_endpoint TAIL_EP = rpc_endpoint("127.0.0.1:11003");

  static std::string pad_str(std::string str, size_t size) {
    str.insert(str.end(), size - str.length(), '\0');
    return str;
  }
};

TEST_F(ReplicationTest, WriteTest) {
  dialog_store store1("/tmp/store1"), store2("/tmp/store2"), store3(
      "/tmp/store3");
  rpc_endpoint ep1(SERVER_EP), ep2(REPLICA1_EP), ep3(TAIL_EP);

  auto server1 = dialog_server::create(&store1, ep1, ep2, ep3);
  auto server2 = dialog_server::create(&store2, ep2, ep3, ep3);
  auto server3 = dialog_server::create(&store3, ep3);

  std::thread serve_thread3([&server3] {
    server3->serve();
  });
  sleep(1);

  std::thread serve_thread2([&server2] {
    server2->serve();
  });
  sleep(1);

  std::thread serve_thread1([&server1] {
    server1->serve();
  });
  sleep(1);

  rpc_dialog_client client(SERVER_ADDRESS, SERVER_PORT);
  client.create_table(
      "my_table",
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg").get_columns(),
      storage::D_IN_MEMORY);

  // Check if all stores have the table
  int64_t table_id = client.get_cur_table_id();
  ASSERT_EQ(table_id, store1.get_table_id("my_table"));
  ASSERT_EQ(table_id, store2.get_table_id("my_table"));
  ASSERT_EQ(table_id, store3.get_table_id("my_table"));

  auto table1 = store1.get_table(table_id), table2 = store2.get_table(table_id),
      table3 = store3.get_table(table_id);

  int64_t ts = utils::time_utils::cur_ns();
  std::string ts_str = std::string(reinterpret_cast<const char*>(&ts), 8);
  client.write(ts_str + pad_str("abc", DATA_SIZE));

  std::string buf1 = std::string(reinterpret_cast<const char*>(table1->read(0)),
  DATA_SIZE);
  ASSERT_EQ(ts, *reinterpret_cast<int64_t*>(&buf1[0]));
  ASSERT_EQ(buf1.substr(8, 3), "abc");

  std::string buf2 = std::string(reinterpret_cast<const char*>(table2->read(0)),
  DATA_SIZE);
  ASSERT_EQ(ts, *reinterpret_cast<int64_t*>(&buf2[0]));
  ASSERT_EQ(buf2.substr(8, 3), "abc");

  std::string buf3 = std::string(reinterpret_cast<const char*>(table3->read(0)),
  DATA_SIZE);
  ASSERT_EQ(ts, *reinterpret_cast<int64_t*>(&buf3[0]));
  ASSERT_EQ(buf3.substr(8, 3), "abc");

  client.disconnect();

  server1->stop();
  if (serve_thread1.joinable()) {
    serve_thread1.join();
  }

  server2->stop();
  if (serve_thread2.joinable()) {
    serve_thread2.join();
  }

  server3->stop();
  if (serve_thread3.joinable()) {
    serve_thread3.join();
  }
}

#endif /* TEST_REPLICATION_TEST_H_ */

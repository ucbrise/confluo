#ifndef RPC_TEST_SERVER_CLIENT_TEST_H_
#define RPC_TEST_SERVER_CLIENT_TEST_H_

#include <thread>

#include "gtest/gtest.h"

#include "conf/configuration_params.h"
#include "rpc_client.h"
#include "rpc_server.h"
#include "rpc_test_utils.h"

using namespace ::confluo::rpc;
using namespace ::confluo;

class ClientConnectionTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;
};

TEST_F(ClientConnectionTest, ConcurrentConnectionsTest) {
  auto store = new confluo_store("/tmp");
  auto server = rpc_server::create(store, SERVER_ADDRESS, SERVER_PORT);
  std::thread serve_thread([&server] {
    server->serve();
  });

  rpc_test_utils::wait_till_server_ready(SERVER_ADDRESS, SERVER_PORT);
  std::vector<rpc_client> clients(4);
  for (auto &client : clients) {
    client.connect(SERVER_ADDRESS, SERVER_PORT);
  }

  for (auto &client : clients) {
    client.disconnect();
  }

  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* RPC_TEST_SERVER_CLIENT_TEST_H_ */

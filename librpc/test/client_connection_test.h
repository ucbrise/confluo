#ifndef TEST_SERVER_CLIENT_TEST_H_
#define TEST_SERVER_CLIENT_TEST_H_

#include "gtest/gtest.h"

#include "configuration_params.h"
#include "rpc_client.h"
#include "rpc_server.h"

using namespace ::confluo::rpc;
using namespace ::confluo;

class ClientConnectionTest : public testing::Test {
};

TEST_F(ClientConnectionTest, ConcurrentConnectionsTest) {
  auto store = new confluo_store("/tmp");
  auto server = rpc_server::create(store, "127.0.0.1", 9090);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  std::vector<rpc_client> clients(configuration_params::MAX_CONCURRENCY);
  for (auto& client : clients) {
    client.connect("127.0.0.1", 9090);
  }

  for (auto& client : clients) {
    client.disconnect();
  }

  server->stop();

  if (serve_thread.joinable()) {
    serve_thread.join();
  }
}

#endif /* TEST_SERVER_CLIENT_TEST_H_ */

#ifndef TEST_SERVER_CLIENT_TEST_H_
#define TEST_SERVER_CLIENT_TEST_H_

#include "configuration_params.h"
#include "dialog_server.h"
#include "gtest/gtest.h"

#include "rpc_dialog_client.h"

using namespace ::dialog::rpc;
using namespace ::dialog;

class ClientConnectionTest : public testing::Test {
 public:
  const std::string SERVER_ADDRESS = "127.0.0.1";
  const int SERVER_PORT = 9090;
  const rpc_endpoint SERVER_EP = rpc_endpoint("127.0.0.1:9090");
};

TEST_F(ClientConnectionTest, ConcurrentConnectionsTest) {
  auto store = new dialog_store("/tmp");
  auto server = dialog_server::create(store, SERVER_EP);
  std::thread serve_thread([&server] {
    server->serve();
  });

  sleep(1);

  std::vector<rpc_dialog_client> clients(configuration_params::MAX_CONCURRENCY);
  for (auto& client : clients) {
    client.connect(SERVER_ADDRESS, SERVER_PORT);
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

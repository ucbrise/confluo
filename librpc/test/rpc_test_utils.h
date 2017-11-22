#ifndef RPC_TEST_RPC_TEST_UTILS_H_
#define RPC_TEST_RPC_TEST_UTILS_H_

#include "rpc_client.h"

class rpc_test_utils {
 public:
  static void wait_till_server_ready(const std::string& host, int port) {
    bool check = true;
    while (check) {
      try {
        confluo::rpc::rpc_client(host, port);
        check = false;
      } catch (TTransportException& e) {
        usleep(100000);
      }
    }
  }
};

#endif /* RPC_TEST_RPC_TEST_UTILS_H_ */

#ifndef RPC_RPC_ENDPOINT_H_
#define RPC_RPC_ENDPOINT_H_

#include "exceptions.h"

namespace dialog {
namespace rpc {

class rpc_endpoint {
 public:
  rpc_endpoint()
      : addr_(""),
        port_(-1) {
  }
  rpc_endpoint(const std::string& endpoint) {
    auto ep_parts = string_utils::split(endpoint, ':');
    if (ep_parts.size() == 2) {
      addr_ = ep_parts[0];
      port_ = std::stoi(ep_parts[1]);
    } else {
      throw parse_exception("Malformed endpoint string " + endpoint);
    }
  }

  const std::string& addr() const {
    return addr_;
  }

  int port() const {
    return port_;
  }

  bool is_valid() const {
    return port_ >= 0;
  }

 private:
  std::string addr_;
  int port_;
};

}
}

#endif /* RPC_RPC_ENDPOINT_H_ */

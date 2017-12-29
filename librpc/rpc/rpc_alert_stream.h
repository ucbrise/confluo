#ifndef RPC_RPC_ALERT_STREAM_H_
#define RPC_RPC_ALERT_STREAM_H_

#include <sstream>

#include "rpc_service.h"

using boost::shared_ptr;

namespace confluo {
namespace rpc {

class rpc_alert_stream {
 public:
  typedef rpc_serviceClient rpc_client;

  rpc_alert_stream(int64_t table_id, shared_ptr<rpc_client> client,
                   rpc_iterator_handle&& handle)
      : table_id_(table_id),
        handle_(std::move(handle)),
        stream_(handle_.data) {
    if (has_more()) {
      std::getline(stream_, alert_);
    }
  }

  const std::string& get() const {
    return alert_;
  }

  rpc_alert_stream& operator++() {
    if (has_more()) {
      if (!std::getline(stream_, alert_) && handle_.has_more) {
        client_->get_more(handle_, table_id_, handle_.desc);
        stream_.str(handle_.data);
      }
    }
    return *this;
  }

  bool has_more() const {
    return !stream_.eof() || handle_.has_more;
  }

  bool empty() const {
    return !has_more();
  }

 private:
  int64_t table_id_;
  rpc_iterator_handle handle_;
  std::stringstream stream_;
  std::string alert_;
  shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_ALERT_STREAM_H_ */

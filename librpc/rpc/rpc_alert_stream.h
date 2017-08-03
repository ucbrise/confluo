#ifndef RPC_RPC_ALERT_STREAM_H_
#define RPC_RPC_ALERT_STREAM_H_

#include <sstream>

#include "dialog_service.h"

using boost::shared_ptr;

namespace dialog {
namespace rpc {

class rpc_alert_stream {
 public:
  typedef dialog_serviceClient rpc_client;

  rpc_alert_stream(shared_ptr<rpc_client> client, rpc_iterator_handle&& handle)
      : handle_(std::move(handle)),
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
        client_->get_more(handle_, handle_.desc);
        stream_.str(handle_.data);
      }
    }
    return *this;
  }

  bool has_more() {
    return handle_.has_more || !stream_.eof();
  }

 private:
  rpc_iterator_handle handle_;
  std::stringstream stream_;
  std::string alert_;
  shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_ALERT_STREAM_H_ */

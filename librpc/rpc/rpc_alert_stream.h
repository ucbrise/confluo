#ifndef RPC_RPC_ALERT_STREAM_H_
#define RPC_RPC_ALERT_STREAM_H_

#include <sstream>

#include "rpc_service.h"

using boost::shared_ptr;

namespace confluo {
namespace rpc {

/**
 * Wrapper for the alert stream
 */
class rpc_alert_stream {
 public:
  typedef rpc_serviceClient rpc_client;

  /**
   * Constructs an alert stream for the rpc client
   *
   * @param table_id The identifier for the table
   * @param client The rpc client
   * @param handle The data for the stream
   */
  rpc_alert_stream(int64_t table_id, shared_ptr<rpc_client> client,
                   rpc_iterator_handle&& handle)
      : table_id_(table_id),
        handle_(std::move(handle)),
        stream_(handle_.data) {
    if (has_more()) {
      std::getline(stream_, alert_);
    }
  }

  /**
   * Gets the alert
   *
   * @return String containing the alert
   */
  const std::string& get() const {
    return alert_;
  }

  /**
   * Advances the alert stream
   *
   * @return This rpc alert stream advanced
   */
  rpc_alert_stream& operator++() {
    if (has_more()) {
      if (!std::getline(stream_, alert_) && handle_.has_more) {
        client_->get_more(handle_, table_id_, handle_.desc);
        stream_.str(handle_.data);
      }
    }
    return *this;
  }

  /**
   * Checks whether there is any more elements in the stream
   *
   * @return True if the stream or handle has any more elements, false
   * otherwise
   */
  bool has_more() const {
    return !stream_.eof() || handle_.has_more;
  }

  /**
   * Checks whether the alert stream is empty
   *
   * @return True if the alert stream is empty, false otherwise
   */
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

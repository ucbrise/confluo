#include "rpc_alert_stream.h"

namespace confluo {
namespace rpc {

rpc_alert_stream::rpc_alert_stream(int64_t table_id,
                                   std::shared_ptr<rpc_alert_stream::rpc_client> client,
                                   rpc_iterator_handle &&handle)
    : table_id_(table_id),
      handle_(std::move(handle)),
      stream_(handle_.data) {
  if (has_more()) {
    std::getline(stream_, alert_);
  }
}

const std::string &rpc_alert_stream::get() const {
  return alert_;
}

rpc_alert_stream &rpc_alert_stream::operator++() {
  if (has_more()) {
    if (!std::getline(stream_, alert_) && handle_.has_more) {
      client_->get_more(handle_, table_id_, handle_.desc);
      stream_.str(handle_.data);
    }
  }
  return *this;
}

bool rpc_alert_stream::has_more() const {
  return !stream_.eof() || handle_.has_more;
}

bool rpc_alert_stream::empty() const {
  return !has_more();
}

}
}
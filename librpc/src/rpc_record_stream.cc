#include "rpc_record_stream.h"

namespace confluo {
namespace rpc {

rpc_record_stream::rpc_record_stream(int64_t multilog_id,
                                     const schema_t &schema,
                                     std::shared_ptr<rpc_record_stream::thrift_client> client,
                                     rpc_iterator_handle &&handle)
    : multilog_id_(multilog_id),
      schema_(schema),
      handle_(std::move(handle)),
      cur_off_(0),
      client_(std::move(client)) {
}

record_t rpc_record_stream::get() {
  return schema_.apply_unsafe(0, &handle_.data[cur_off_]);
}

rpc_record_stream &rpc_record_stream::operator++() {
  if (has_more()) {
    cur_off_ += schema_.record_size();
    if (cur_off_ == handle_.data.size() && handle_.has_more) {
      client_->get_more(handle_, multilog_id_, handle_.desc);
      cur_off_ = 0;
    }
  }
  return *this;
}

bool rpc_record_stream::has_more() const {
  return handle_.has_more || cur_off_ != handle_.data.size();
}

bool rpc_record_stream::empty() const {
  return !has_more();
}

}
}
#ifndef RPC_RPC_RECORD_STREAM_H_
#define RPC_RPC_RECORD_STREAM_H_

#include "rpc_service.h"

using boost::shared_ptr;

namespace confluo {
namespace rpc {

class rpc_record_stream {
 public:
  typedef rpc_serviceClient thrift_client;

  rpc_record_stream(int64_t multilog_id, const schema_t& schema,
                    shared_ptr<thrift_client> client,
                    rpc_iterator_handle&& handle)
      : multilog_id_(multilog_id),
        schema_(schema),
        handle_(std::move(handle)),
        cur_off_(0),
        client_(std::move(client)) {
  }

  record_t get() {
    return schema_.apply_unsafe(0, &handle_.data[cur_off_]);
  }

  rpc_record_stream& operator++() {
    if (has_more()) {
      cur_off_ += schema_.record_size();
      if (cur_off_ == handle_.data.size() && handle_.has_more) {
        client_->get_more(handle_, multilog_id_, handle_.desc);
        cur_off_ = 0;
      }
    }
    return *this;
  }

  bool has_more() const {
    return handle_.has_more || cur_off_ != handle_.data.size();
  }

  bool empty() const {
    return !has_more();
  }

 private:
  int64_t multilog_id_;
  schema_t schema_;
  rpc_iterator_handle handle_;
  size_t cur_off_;
  shared_ptr<thrift_client> client_;
};

}
}

#endif /* RPC_RPC_RECORD_STREAM_H_ */

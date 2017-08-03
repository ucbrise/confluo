#ifndef RPC_RPC_RECORD_STREAM_H_
#define RPC_RPC_RECORD_STREAM_H_

#include "dialog_service.h"

using boost::shared_ptr;

namespace dialog {
namespace rpc {

class rpc_record_stream {
 public:
  typedef dialog_serviceClient rpc_client;

  rpc_record_stream(const schema_t& schema, shared_ptr<rpc_client> client,
                    rpc_iterator_handle&& handle)
      : schema_(schema),
        handle_(std::move(handle)),
        cur_off_(0) {
  }

  record_t get() {
    return record_t(0, &handle_.data[cur_off_], schema_.size());
  }

  rpc_record_stream& operator++() {
    if (has_more()) {
      cur_off_ += schema_.record_size();
      if (cur_off_ == handle_.data.size() && handle_.has_more) {
        client_->get_more(handle_, handle_.desc);
        cur_off_ = 0;
      }
    }
    return *this;
  }

  bool has_more() {
    return handle_.has_more || cur_off_ != handle_.data.size();
  }

 private:
  schema_t schema_;
  rpc_iterator_handle handle_;
  size_t cur_off_;
  shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_RECORD_STREAM_H_ */

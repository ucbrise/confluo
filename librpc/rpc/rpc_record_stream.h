#ifndef RPC_RPC_RECORD_STREAM_H_
#define RPC_RPC_RECORD_STREAM_H_

#include "dialog_service.h"

using boost::shared_ptr;

namespace dialog {
namespace rpc {

class rpc_record_stream {
 public:
  typedef dialog_serviceClient rpc_client;

  rpc_record_stream(int64_t table_id, const schema_t& schema,
                    shared_ptr<rpc_client> client, rpc_iterator_handle&& handle)
      : table_id_(table_id),
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
        client_->get_more(handle_, table_id_, handle_.desc);
        cur_off_ = 0;
      }
    }
    return *this;
  }

  bool has_more() {
    return handle_.has_more || cur_off_ != handle_.data.size();
  }

 private:
  int64_t table_id_;
  schema_t schema_;
  rpc_iterator_handle handle_;
  size_t cur_off_;
  shared_ptr<rpc_client> client_;
};

}
}

#endif /* RPC_RPC_RECORD_STREAM_H_ */

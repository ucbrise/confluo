#ifndef RPC_RPC_RECORD_STREAM_H_
#define RPC_RPC_RECORD_STREAM_H_

#include "rpc_service.h"

using boost::shared_ptr;

namespace confluo {
namespace rpc {

/**
 * Container for a stream of records
 */
class rpc_record_stream {
 public:
  typedef rpc_serviceClient thrift_client;

  /**
   * Constructs a record stream from the specified parameters
   *
   * @param multilog_id The identifier of the atomic multilog
   * @param schema The schema for the records
   * @param client The thrift client
   * @param handle The r value handle iterator
   */
  rpc_record_stream(int64_t multilog_id, const schema_t& schema,
                    shared_ptr<thrift_client> client,
                    rpc_iterator_handle&& handle)
      : multilog_id_(multilog_id),
        schema_(schema),
        handle_(std::move(handle)),
        cur_off_(0),
        client_(std::move(client)) {
  }

  /**
   * Gets the record data from the schema
   *
   * @return The record at the offset
   */
  record_t get() {
    return schema_.apply_unsafe(0, &handle_.data[cur_off_]);
  }

  /**
   * Advances the stream to get the next record
   *
   * @return This record stream advanced
   */
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

  /**
   * Checks to see if the record stream has any more elements
   *
   * @return True if there are any elements left, false otherwise
   */
  bool has_more() const {
    return handle_.has_more || cur_off_ != handle_.data.size();
  }

  /**
   * Checks to see if the record stream is empty
   *
   * @return True if the record stream is empty, false otherwise
   */
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

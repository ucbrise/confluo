#ifndef EXAMPLES_STREAM_PRODUCER_H_
#define EXAMPLES_STREAM_PRODUCER_H_

#include <math.h>
#include "atomic_multilog.h"
#include "rpc_client.h"
#include "rpc_record_batch_builder.h"

namespace confluo {

class stream_producer : public rpc::rpc_client {
 public:
  stream_producer(const std::string server_address, int server_port,
                  size_t batch_size, uint64_t buffer_timeout_ms)
      : rpc_client(server_address, server_port),
        batch_size_(batch_size),
        last_flush_timestamp_ms_(utils::time_utils::cur_ms()),
        buffer_timeout_ms_(buffer_timeout_ms),
        batch_builder_(cur_schema_) {
  }

  void buffer(const rpc::record_data& record) {
    if (cur_multilog_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }

    batch_builder_.add_record(record);
    uint64_t elapsed_time = utils::time_utils::cur_ms()
        - last_flush_timestamp_ms_;

    if (batch_builder_.num_records() >= batch_size_
        || elapsed_time > buffer_timeout_ms_) {
      flush();
      last_flush_timestamp_ms_ = utils::time_utils::cur_ms();
    }
  }

  void flush() {
    if (batch_builder_.num_records() > 0) {
      client_->append_batch(cur_multilog_id_, batch_builder_.get_batch());
    }

    if (batch_builder_.num_records() != 0) {
      throw illegal_state_exception("Write buffer was not cleared after flush");
    }
  }

  void disconnect() {
    rpc::rpc_client::disconnect();
    flush();
  }

 private:
  size_t batch_size_;
  uint64_t last_flush_timestamp_ms_;
  uint64_t buffer_timeout_ms_;
  rpc::rpc_record_batch_builder batch_builder_;  // Write buffer
};

}

#endif /* EXAMPLES_STREAM_PRODUCER_H_ */

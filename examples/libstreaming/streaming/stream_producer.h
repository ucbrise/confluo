#ifndef EXAMPLES_STREAM_PRODUCER_H_
#define EXAMPLES_STREAM_PRODUCER_H_

#include <math.h>
#include "atomic_multilog.h"
#include "rpc_client.h"
#include "rpc_configuration_params.h"

namespace confluo {

class stream_producer : public rpc::rpc_client {
 public:
  stream_producer(const std::string server_address, const int server_port,
          uint64_t buffer_timeout_ms) : 
      rpc_client(server_address, server_port) {
        last_flush_timestamp_ms_ = utils::time_utils::cur_ms();
        buffer_timeout_ms_ = buffer_timeout_ms;
  }

  void buffer(const std::string& record) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }

    builder_.add_record(record);
    uint64_t elapsed_time = utils::time_utils::cur_ms() - 
        last_flush_timestamp_ms_;

    if (builder_.num_records() >= rpc::rpc_configuration_params::WRITE_BATCH_SIZE|| elapsed_time > buffer_timeout_ms_) {
      flush();
      last_flush_timestamp_ms_ = utils::time_utils::cur_ms();
    }
  }
  
  void flush() {
    if (builder_.num_records() > 0) {
      client_->append_batch(cur_table_id_, builder_.get_batch());
    }

    if (builder_.num_records() != 0) {
        throw illegal_state_exception("Write buffer was not cleared after flush");
    }
  }

  void disconnect() {
    rpc::rpc_client::disconnect();
    flush();
  }

 private:
  uint64_t last_flush_timestamp_ms_;
  uint64_t buffer_timeout_ms_;

};

}

#endif /* EXAMPLES_STREAM_PRODUCER_H_ */

#ifndef EXAMPLES_STREAM_PRODUCER_H_
#define EXAMPLES_STREAM_PRODUCER_H_

#include <math.h>
#include "atomic_multilog.h"
#include "rpc_client.h"
#include "rpc_configuration_params.h"

namespace confluo {

class stream_producer : public rpc::rpc_client {
 public:
  stream_producer(const std::string SERVER_ADDRESS, const int SERVER_PORT,
          uint64_t buffer_timeout_ms) : 
      rpc_client(SERVER_ADDRESS, SERVER_PORT) {
        last_time = utils::time_utils::cur_ms();
        buffer_timeout_ms_ = buffer_timeout_ms;
        writes = 0;

  }

  void buffer(const std::string& record) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }

    builder_.add_record(record);
    uint64_t elapsed_time = utils::time_utils::cur_ms() - last_time;

    if (builder_.num_records() >= rpc::rpc_configuration_params::WRITE_BATCH_SIZE|| elapsed_time > buffer_timeout_ms_) {
      flush();
      writes++;
      last_time = utils::time_utils::cur_ms();
    }
  }
  
  void flush() {
    if (builder_.num_records() > 0) {
      client_->append_batch(cur_table_id_, builder_.get_batch());
    }
  }

  uint64_t get_write_ops() {
      return writes;
  }
  
 private:
  uint64_t last_time;
  uint64_t buffer_timeout_ms_;
  uint64_t writes;

};

}

#endif /* EXAMPLES_STREAM_PRODUCER_H_ */

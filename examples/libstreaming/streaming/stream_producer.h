#ifndef EXAMPLES_STREAM_PRODUCER_H_
#define EXAMPLES_STREAM_PRODUCER_H_

#include <math.h>
#include "atomic_multilog.h"
#include "rpc_configuration_params.h"

namespace confluo {

class stream_producer {
 public:
  stream_producer(atomic_multilog* dtable) {
        dtable_ = dtable;
        last_time = utils::time_utils::cur_ns();
        MAX_ELAPSED_TIME = 1e9;
  }

  void buffer(const std::string& record) {
      if (dtable_->get_name().empty()) {
          throw illegal_state_exception("Must set table first");
      }

      builder.add_record(record);
      uint64_t current_time = utils::time_utils::cur_ns();
      uint64_t elapsed_time = current_time - last_time;

      record_batch batch = builder.get_batch();
      if (batch.nrecords >= 
              rpc::rpc_configuration_params::WRITE_BATCH_SIZE ||
              elapsed_time > MAX_ELAPSED_TIME) {
          log_offsets.push_back(dtable_->append_batch(batch));
          last_time = current_time;
          flush();
      }
  }

  void flush() {
      builder = record_batch_builder();
  }

  std::vector<size_t> get_log_offsets() {
      return log_offsets;
  }

 private:
  atomic_multilog* dtable_;
  record_batch_builder builder;
  uint64_t last_time;
  uint64_t MAX_ELAPSED_TIME;
  std::vector<size_t> log_offsets;

};

}

#endif /* EXAMPLES_STREAM_PRODUCER_H_ */

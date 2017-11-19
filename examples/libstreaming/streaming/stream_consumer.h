#ifndef EXAMPLES_STREAM_CONSUMER_H_
#define EXAMPLES_STREAM_CONSUMER_H_

#include <math.h>
#include <map>
#include "atomic_multilog.h"

namespace confluo {

class stream_consumer {
 public:
  stream_consumer(atomic_multilog* dtable) {
      dtable_ = dtable;
      READ_BATCH_SIZE = 8;
  }

  void read(std::string& _return, uint64_t offset, size_t record_size) {
      if (dtable_->get_name().empty()) {
          throw illegal_state_exception("Must set table first");
      }

      if (read_buffer_.find(offset) == read_buffer_.end()) {
        ro_data_ptr data;

        for (uint64_t i = 0; i < READ_BATCH_SIZE; i++) {
            uint64_t map_offset = offset + i * record_size;
            dtable_->read(map_offset, data);
            char* rbuf = (char*) data.get();

            std::string result = std::string(
                    reinterpret_cast<const char*>(rbuf), record_size);
            read_buffer_[map_offset] = result;
        }
      }
      _return = read_buffer_[offset];
  }

 private:
  atomic_multilog* dtable_;
  std::map<uint64_t, std::string> read_buffer_;
  uint64_t READ_BATCH_SIZE;

};

}

#endif /* EXAMPLES_STREAM_CONSUMER_H_ */

#ifndef EXAMPLES_STREAM_CONSUMER_H_
#define EXAMPLES_STREAM_CONSUMER_H_

#include <math.h>
#include <map>

#include "rpc_client.h"
#include "atomic_multilog.h"

namespace confluo {

class stream_consumer : public rpc::rpc_client {
 public:
  stream_consumer(const std::string SERVER_ADDRESS, const int SERVER_PORT,
          uint64_t consumer_prefetch_size) : 
      rpc_client(SERVER_ADDRESS, SERVER_PORT) {
      consumer_prefetch_size_ = consumer_prefetch_size;
      reads = 0;
  }

  void read_seq(std::string& _return, int64_t offset, size_t record_size) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    int64_t& buf_off = read_buffer_.first;
    std::string& buf = read_buffer_.second;
    int64_t rbuf_lim = buf_off + buf.size();
    if (buf_off == -1 || offset < buf_off || offset >= rbuf_lim) {
      read_buffer_.first = offset;
      client_->read(buf, cur_table_id_, buf_off, consumer_prefetch_size_);
      reads++;
    }
    _return = buf.substr(offset - buf_off, record_size);
  }

  uint64_t num_read_ops() {
      return reads;
  }
  

 private:
  uint64_t consumer_prefetch_size_;
  uint64_t reads;

};

}

#endif /* EXAMPLES_STREAM_CONSUMER_H_ */

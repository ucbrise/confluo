#ifndef EXAMPLES_STREAM_CONSUMER_H_
#define EXAMPLES_STREAM_CONSUMER_H_

#include <math.h>
#include <map>

#include "rpc_client.h"
#include "atomic_multilog.h"

namespace confluo {

class stream_consumer : public rpc::rpc_client {
 public:
  stream_consumer(const std::string server_address, const int server_port,
          uint64_t prefetch_size) : 
      rpc_client(server_address, server_port) {
      prefetch_size_ = prefetch_size;
      offset_ = 0;
  }

  void consume(std::string& _return) {
    if (cur_table_id_ == -1) {
      throw illegal_state_exception("Must set table first");
    }
    int64_t& buf_off = read_buffer_.first;
    std::string& buf = read_buffer_.second;
    int64_t rbuf_lim = buf_off + buf.size();
    if (buf_off == -1 || offset_ < buf_off || offset_ >= rbuf_lim) {
      read_buffer_.first = offset_;
      client_->read(buf, cur_table_id_, buf_off, prefetch_size_);
    }
    _return = buf.substr(offset_ - buf_off, cur_schema_.record_size());
    offset_ += cur_schema_.record_size();
  }

 private:
  uint64_t prefetch_size_;
  int64_t offset_;

};

}

#endif /* EXAMPLES_STREAM_CONSUMER_H_ */

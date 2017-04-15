#ifndef STREAMING_CONSUMER_H_
#define STREAMING_CONSUMER_H_

#include <vector>

#include "ss_client.h"
#include "assertions.h"

namespace streaming {

class consumer : public ss_client {
 public:
  consumer()
      : ss_client(),
        uuid_(1),
        off_(0),
        batch_bytes_(1024) {
  }

  consumer(uuid_t uuid, size_t batch_bytes, const std::string& hostname,
           const int port)
      : ss_client(hostname, port),
        uuid_(uuid),
        off_(0),
        batch_bytes_(batch_bytes) {
  }

  void set_uuid(uuid_t uuid) {
    uuid_ = uuid;
  }

  void set_batch_bytes(size_t batch_bytes) {
    batch_bytes_ = batch_bytes;
  }

  void recv(std::vector<std::string>& batch) {
    std::string buf = "";
    while (buf.length() == 0)
      client_->read(buf, uuid_, off_, batch_bytes_);

    // De-serialize batch
    size_t boff = 0;
    size_t blen = buf.length();
    char* bbuf = &buf[0];
    while (boff + sizeof(uint32_t) < blen) {
      size_t rlen = *((uint32_t*) (bbuf + boff));
      boff += sizeof(uint32_t);
      if (boff + rlen > blen) {
        boff -= sizeof(uint32_t);
        break;
      }
      batch.push_back(std::string(bbuf + boff, rlen));
      boff += rlen;
    }
    off_ += boff;
  }

 private:
  uuid_t uuid_;
  offset_t off_;
  size_t batch_bytes_;
};

}

#endif /* STREAMING_CONSUMER_H_ */

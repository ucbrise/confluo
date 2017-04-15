#ifndef STREAMING_PRODUCER_H_
#define STREAMING_PRODUCER_H_

#include <vector>

#include "ss_client.h"
#include "assertions.h"

namespace streaming {

class producer : public ss_client {
 public:
  producer()
      : ss_client(),
        uuid_(1) {
  }

  producer(uuid_t uuid, const std::string& hostname, const int port)
      : ss_client(hostname, port) {
    set_uuid(uuid);
  }

  void set_uuid(uuid_t uuid) {
    uuid_ = uuid;
    client_->add_stream(uuid_);
  }

  void send(const std::vector<std::string>& batch) {
    // Serialize batch
    size_t blen = batch.size() * sizeof(uint32_t);
    for (const std::string& record : batch)
      blen += record.length();

    std::string data(blen, 0);
    char* bbuf = &data[0];
    size_t boff = 0;
    for (const std::string& record : batch) {
      *((uint32_t*) (bbuf + boff)) = record.length();
      fprintf(stderr, "[Before]Record length = %zu\n", record.length());
      boff += sizeof(uint32_t);
      memcpy(bbuf + boff, record.c_str(), record.length());
      boff += record.length();
    }

    // De-serialize batch
    boff = 0;
    bbuf = &data[0];
    fprintf(stderr, "Deserializing batch...\n");
    while (boff + sizeof(uint32_t) < blen) {
      size_t rlen = *((uint32_t*) (bbuf + boff));
      fprintf(stderr, "[After]Record length = %zu\n", rlen);
      boff += sizeof(uint32_t);
      if (boff + rlen > blen) {
        boff -= sizeof(uint32_t);
        break;
      }
      boff += rlen;
    }
    assert_throw(data.length() > 0U, "Empty write");
    client_->write(uuid_, data);
  }

 private:
  uuid_t uuid_;
};

}

#endif /* STREAMING_PRODUCER_H_ */

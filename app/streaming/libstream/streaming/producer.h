#ifndef STREAMING_PRODUCER_H_
#define STREAMING_PRODUCER_H_

#include <vector>

#include "ss_client.h"

namespace streaming {

class producer : public ss_client {
 public:
  producer()
      : ss_client(),
        uuid_(1) {
  }

  producer(uuid_t uuid, const std::string& hostname, const int port)
      : ss_client(hostname, port) {
    uuid_ = uuid;
    client_->add_stream(uuid_);
  }

  void set_uuid(uuid_t uuid) {
    uuid_ = uuid;
  }

  void send(const std::vector<std::string>& batch) {
    // Serialize batch
    size_t size = batch.size() * sizeof(uint32_t);
    for (const std::string& record : batch)
      size += record.length();

    std::string data(size, 0);
    char* buf = &data[0];
    for (const std::string& record : batch) {
      uint32_t record_size = record.length();
      memcpy(buf, (char*) (&record_size), sizeof(uint32_t));
      buf += sizeof(uint32_t);
      memcpy(buf, record.c_str(), record_size);
    }
    client_->write(uuid_, data);
  }

 private:
  uuid_t uuid_;
};

}

#endif /* STREAMING_PRODUCER_H_ */

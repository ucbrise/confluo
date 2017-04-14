#ifndef STREAMING_STREAM_DB_H_
#define STREAMING_STREAM_DB_H_

#include "monolog.h"
#include "stream_partition.h"
#include "server/stream_service_types.h"

namespace streaming {

class stream_db {
 public:
  typedef stream_partition* partition_ref;

  stream_db(const std::string& data_path)
      : data_path_(data_path) {
  }

  void add_stream(uuid_t uuid) {
    if (partitions_[uuid] == nullptr) {
      partitions_[uuid] = new stream_partition(
          data_path_ + "part." + std::to_string(uuid));
    }
  }

  partition_ref& operator[](uuid_t uuid) {
    return partitions_[uuid];
  }

 private:
  std::string data_path_;
  monolog::monolog_base<partition_ref> partitions_;
};

}

#endif /* STREAMING_STREAM_DB_H_ */

#ifndef STREAMING_STREAM_DB_H_
#define STREAMING_STREAM_DB_H_

#include <mutex>

#include "monolog.h"
#include "stream_partition.h"
#include "server/stream_service_types.h"
#include "logger.h"

namespace streaming {

class stream_db {
 public:
  typedef stream_partition* partition_ref;

  stream_db(const std::string& data_path)
      : data_path_(data_path) {
    LOG_INFO<<"Initializing stream_db...";
  }

  void add_stream(uuid_t uuid) {
    std::lock_guard<std::mutex> lock(m_);
    if (partitions_[uuid] == nullptr) {
      LOG_INFO << "Stream does not exist";
      partitions_[uuid] = new stream_partition(
          data_path_ + "part." + std::to_string(uuid));
    } else {
      LOG_INFO << "Stream already exists";
    }
  }

  partition_ref& operator[](uuid_t uuid) {
    return partitions_[uuid];
  }

private:
  std::mutex m_;
  std::string data_path_;
  monolog::monolog_base<partition_ref> partitions_;
};

}

#endif /* STREAMING_STREAM_DB_H_ */

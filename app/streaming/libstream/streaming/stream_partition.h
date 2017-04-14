#ifndef STREAMING_STREAM_PARTITION_H_
#define STREAMING_STREAM_PARTITION_H_

#include "log_store.h"
#include "server/stream_service_types.h"

namespace streaming {

class stream_partition {
 public:
  stream_partition(const std::string& data_path)
      : store_(create_dir(data_path)) {
  }

  offset_t write(const std::string& batch) {
    return store_.append(batch);
  }

  void read(std::string& data, const offset_t offset, length_t length) {
    data.resize(length);
    store_.get(offset, (uint8_t*) &data[0], length);
  }

 private:
  std::string create_dir(const std::string& data_path) {
    struct stat st = { 0 };
    if (stat(data_path.c_str(), &st) == -1)
      mkdir(data_path.c_str(), 0777);
    return data_path;
  }

  datastore::append_only::log_store<datastore::append_only::durable_relaxed,
      datastore::append_only::write_stalled> store_;
};

}

#endif /* STREAMING_STREAM_PARTITION_H_ */

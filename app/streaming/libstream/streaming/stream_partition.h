#ifndef STREAMING_STREAM_PARTITION_H_
#define STREAMING_STREAM_PARTITION_H_

#include "assertions.h"
#include "log_store.h"
#include "server/stream_service_types.h"

namespace streaming {

class stream_partition {
 public:
  stream_partition(const std::string& data_path)
      : read_tail_(UINT64_C(0)) {
    data_log_.init("data", create_dir(data_path));
  }

  offset_t write(const std::string& batch) {
    uint64_t tail = data_log_.append((const uint8_t*) batch.c_str(),
                                     batch.length());
    update_read_tail(tail, batch.length());
    return tail;
  }

  void read(std::string& data, const offset_t offset, length_t length) {
    if (offset >= atomic::load(&read_tail_))
      return;
    data.resize(length);
    data_log_.read(offset, (uint8_t*) &data[0], length);
  }

 private:
  void update_read_tail(offset_t tail, offset_t len) {
    offset_t old_tail = tail;
    while (!atomic::weak::cas(&read_tail_, &old_tail, tail + len)) {
      old_tail = tail;
      std::this_thread::yield();
    }
  }

  std::string create_dir(const std::string& data_path) {
    struct stat st = { 0 };
    if (stat(data_path.c_str(), &st) == -1)
      mkdir(data_path.c_str(), 0777);
    return data_path;
  }

  atomic::type<offset_t> read_tail_;

  monolog::mmap_monolog_relaxed<uint8_t, 65536, 1073741824> data_log_;
};

}

#endif /* STREAMING_STREAM_PARTITION_H_ */

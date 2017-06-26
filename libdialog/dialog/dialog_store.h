#ifndef DIALOG_DIALOG_STORE_H_
#define DIALOG_DIALOG_STORE_H_

#include <functional>
#include <numeric>
#include <thread>

#include "storage.h"
#include "monolog.h"
#include "serde.h"

namespace dialog {

class read_tail {
 public:
  read_tail()
      : read_tail_(UINT64_C(0)) {
  }

  uint64_t get() const {
    return atomic::load(&read_tail_);
  }

  bool is_valid(uint64_t id, uint64_t tail) const {
    return id < tail;
  }

  void update(uint64_t old_tail, uint64_t bytes) {
    uint64_t expected = old_tail;
    while (!atomic::weak::cas(&read_tail_, &expected, old_tail + bytes)) {
      expected = old_tail;
      std::this_thread::yield();
    }
  }

 private:
  atomic::type<uint64_t> read_tail_;
};

template<class storage_mode = storage::in_memory>
class dialog_store {
 public:
  dialog_store() = default;

  dialog_store(const std::string& path) {
    data_log_.init(path, "data_log");
  }

  uint64_t append(const uint8_t* data, size_t bytes) {
    uint64_t offset = data_log_.append(data, bytes);
    data_log_.flush(offset, bytes);
    cc_.update(offset, bytes);
    return offset;
  }

  template<typename T>
  uint64_t append(const T& data) {
    uint64_t length = serializer<T>::size(data);
    uint64_t offset = data_log_.reserve(length);
    serializer<T>::serialize(data_log_.ptr(offset), data);
    data_log_.flush(offset, length);
    cc_.update(offset, length);
    return offset;
  }

  uint64_t append_batch(const std::vector<uint8_t*>& batch,
                        std::vector<size_t>& lengths) {
    size_t length = std::accumulate(lengths.begin(), lengths.end(), 0);
    uint64_t offset = data_log_.reserve(length);
    uint64_t off = offset;
    for (size_t i = 0; i < batch.size(); i++) {
      data_log_.write(off, batch.at(i), lengths.at(i));
      off += lengths.at(i);
    }
    data_log_.flush(offset, length);
    cc_.update(offset, length);
    return offset;
  }

  template<typename T>
  uint64_t append_batch(const std::vector<T>& batch) {
    size_t length = 0;
    std::vector<size_t> batch_offsets;
    for (size_t i = 0; i < batch.size(); i++) {
      batch_offsets.push_back(length);
      length += serializer<T>::size(batch.at(i));
    }
    uint64_t offset = data_log_.reserve(length);
    for (size_t i = 0; i < batch.size(); i++) {
      serializer<T>::serialize(data_log_.ptr(offset + batch_offsets.at(i)),
                               batch.at(i));
    }
    data_log_.flush(offset, length);
    cc_.update(offset, length);
    return offset;
  }

  bool read(uint64_t offset, uint8_t* data, size_t length,
            uint64_t tail) const {
    if (cc_.is_valid(offset, tail)) {
      data_log_.read(offset, data, length);
      return true;
    }
    return false;
  }

  template<typename T>
  bool read(uint64_t offset, T* data, uint64_t tail) const {
    if (cc_.is_valid(offset, tail)) {
      deserializer<T>::deserialize(data_log_.cptr(offset), data);
      return true;
    }
    return false;
  }

  bool get(uint64_t offset, uint8_t* data, size_t length) const {
    uint64_t tail = cc_.get();
    return read(offset, data, length, tail);
  }

  template<typename T>
  bool get(uint64_t offset, T* data) const {
    uint64_t tail = cc_.get();
    return read(offset, data, tail);
  }

  size_t num_records() const {
    return cc_.get();
  }

 protected:
  dialog::monolog::monolog_linear<uint8_t, 65536, 1073741824> data_log_;
  read_tail cc_;
};

}

#endif /* DIALOG_DIALOG_STORE_H_ */

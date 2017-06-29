#ifndef DIALOG_DIALOG_STORE_H_
#define DIALOG_DIALOG_STORE_H_

#include <functional>
#include <numeric>
#include <thread>

#include "storage.h"
#include "monolog.h"
#include "serde.h"

#include "attributes.h"

using namespace ::dialog::monolog;

namespace dialog {

template<typename storage_mode = storage::in_memory>
class read_tail {
 public:
  read_tail() {
    read_tail_ = nullptr;
  }

  read_tail(const std::string& data_path) {
    init(data_path);
  }

  void init(const std::string& data_path) {
    read_tail_ = (atomic::type<uint64_t>*) storage_mode::allocate(
        data_path + "/read_tail", sizeof(uint64_t));
  }

  uint64_t get() const {
    return atomic::load(read_tail_);
  }

  void advance(uint64_t old_tail, uint32_t bytes) {
    uint64_t expected = old_tail;
    while (!atomic::weak::cas(read_tail_, &expected, old_tail + bytes)) {
      expected = old_tail;
      std::this_thread::yield();
    }
    storage_mode::flush(read_tail_, sizeof(uint64_t));
  }

 private:
  atomic::type<uint64_t>* read_tail_;
};

template<class storage_mode = storage::in_memory>
class dialog_store {
 public:
  dialog_store()
      : rt_("") {
  }

  dialog_store(const std::string& path)
      : rt_(path) {
    data_log_.init("data_log", path);
  }

  uint64_t append(const uint8_t* data, size_t length) {
    uint64_t offset = data_log_.append(data, length);
    data_log_.flush(offset, length);
    rt_.advance(offset, length);
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
    rt_.advance(offset, length);
    return offset;
  }

  void* ptr(uint64_t offset, uint64_t tail) const {
    if (offset < tail)
      return data_log_.cptr(offset);
    return nullptr;
  }

  void* ptr(uint64_t offset) const {
    return ptr(offset, rt_.get());
  }

  bool read(uint64_t offset, uint8_t* data, size_t length,
            uint64_t tail) const {
    if (offset < tail) {
      data_log_.read(offset, data, length);
      return true;
    }
    return false;
  }

  bool get(uint64_t offset, uint8_t* data, size_t length) const {
    return read(offset, data, length, rt_.get());
  }

  size_t num_records() const {
    return rt_.get();
  }

 protected:
  monolog_linear<uint8_t, 65536, 1073741824, 1048576, storage_mode> data_log_;
  read_tail<storage_mode> rt_;
};

}

#endif /* DIALOG_DIALOG_STORE_H_ */

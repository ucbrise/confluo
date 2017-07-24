#ifndef DIALOG_READ_TAIL_H_
#define DIALOG_READ_TAIL_H_

#include <cstdint>
#include <thread>

#include "atomic.h"
#include "storage.h"

namespace dialog {

class read_tail {
 public:
  read_tail() {
    read_tail_ = nullptr;
    storage_ = storage::IN_MEMORY;
  }

  read_tail(const std::string& data_path,
            const storage::storage_mode& storage) {
    init(data_path, storage);
  }

  void init(const std::string& data_path,
            const storage::storage_mode& storage) {
    storage_ = storage;
    read_tail_ = (atomic::type<uint64_t>*) storage_.allocate(
        data_path + "/read_tail", sizeof(uint64_t));
    atomic::store(read_tail_, UINT64_C(0));
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
    storage_.flush(read_tail_, sizeof(uint64_t));
  }

 private:
  atomic::type<uint64_t>* read_tail_;
  storage::storage_mode storage_;
};

}

#endif /* DIALOG_READ_TAIL_H_ */

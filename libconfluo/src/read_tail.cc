#include "read_tail.h"
#include "io_utils.h"

namespace confluo {

read_tail::read_tail() {
  read_tail_ = nullptr;
  mode_ = storage::IN_MEMORY;
}

read_tail::read_tail(const std::string &data_path, const storage::storage_mode &mode, bool load) {
  init(data_path, mode, load);
}

void read_tail::init(const std::string &data_path, const storage::storage_mode &mode, bool load) {
  mode_ = mode;
  auto path = data_path + "/read_tail";
  uint64_t value = 0;
  if (load) {
    std::ifstream rt_ifstream(path);
    value = io_utils::read<uint64_t>(rt_ifstream);
  }
  auto storage_func = storage::storage_mode_functions::STORAGE_FNS()[mode_];
  read_tail_ = (atomic::type<uint64_t> *) storage_func.allocate(path, sizeof(uint64_t));
  atomic::store(read_tail_, UINT64_C(value));
}

uint64_t read_tail::get() const {
  return atomic::load(read_tail_);
}

void read_tail::advance(uint64_t old_tail, uint32_t bytes) {
  uint64_t expected = old_tail;
  while (!atomic::weak::cas(read_tail_, &expected, old_tail + bytes)) {
    expected = old_tail;
    std::this_thread::yield();
  }
  storage::storage_mode_functions::STORAGE_FNS()[mode_].flush(read_tail_, sizeof(uint64_t));
}

}
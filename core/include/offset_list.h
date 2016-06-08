#ifndef OFFSET_LIST_H_
#define OFFSET_LIST_H_

#include "flags.h"
#include "locks.h"

class ConcurrentOffsetList {
 public:
  ConcurrentOffsetList() {
  }

  void push_back(const uint32_t val) {
    WriteLock write_guard(mtx_);
    offsets_.push_back(val);
  }

  uint32_t at(const size_t i) {
    ReadLock read_guard(mtx_);
    return offsets_.at(i);
  }

  size_t size() {
    ReadLock read_guard(mtx_);
    return offsets_.size();
  }

  void snapshot(std::vector<uint32_t> &out) {
    ReadLock read_guard(mtx_);
    out = offsets_;
  }

  std::vector<uint32_t>& vector() {
    return offsets_;
  }

 private:
  std::vector<uint32_t> offsets_;
  Mutex mtx_;
};

typedef ConcurrentOffsetList OffsetList;

#endif /* OFFSET_LIST_H_ */

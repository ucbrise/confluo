#ifndef DIALOG_DIALOG_MEMPOOL_H_
#define DIALOG_DIALOG_MEMPOOL_H_

#include <typeinfo>
#include "aggregated_reflog.h"
#include "atomic.h"
#include "configuration_params.h"
#include "mempool_stat.h"

namespace dialog {
namespace mempool {

template<typename T, size_t ARRAY_LEN>
class mempool {

 public:
  mempool(mempool_stat* stat) :
    block_size_(ARRAY_LEN * sizeof(T)),
    stat_(stat)
    {
  }

  T* alloc() {
    if (stat_->get() >= MAX_MEMORY) {
      THROW(max_memory_exceeded_exception, "Max memory reached!");
    }
    char* ptr = new char[block_size_];
    T* tptr = new(ptr) T;
    stat_->increment(block_size_);
    return tptr;
  }

  void dealloc(T* ptr) {
    ptr->~T();
    delete[] ptr;
    stat_->decrement(block_size_);
  }

 private:
  static const size_t MAX_MEMORY;
  const size_t block_size_;
  mempool_stat* stat_;

};

template<typename T, size_t BLOCK_SIZE>
const size_t mempool<T, BLOCK_SIZE>::MAX_MEMORY = 1e6; // configuration_params::MAX_MEMORY <-- doesn't work;

static mempool_stat* STAT = new mempool_stat();

}
}

#endif /* DIALOG_DIALOG_MEMPOOL_H_ */

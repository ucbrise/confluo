#ifndef DIALOG_DIALOG_MEMPOOL_H_
#define DIALOG_DIALOG_MEMPOOL_H_

#include <typeinfo>
#include "atomic.h"
#include "configuration_params.h"
#include "mempool_stat.h"

namespace dialog {

template<typename T, size_t BLOCK_SIZE>
class mempool {

 public:
  mempool() :
    array_size_(BLOCK_SIZE / sizeof(T)),
    max_memory_(configuration_params::MAX_MEMORY())
    {
  }

  T* alloc() {
    if (STAT.get() >= max_memory_) {
      THROW(mempool_exception, "Max memory reached!");
    }
    T* ptr = new T[array_size_];
    STAT.increment(BLOCK_SIZE);
    return ptr;
  }

  void dealloc(T* ptr) {
    delete[] ptr;
    STAT.decrement(BLOCK_SIZE);
  }

 private:
  static mempool_stat STAT;
  const int array_size_;
  const size_t max_memory_;

};

template<typename T, size_t BLOCK_SIZE>
mempool_stat mempool<T, BLOCK_SIZE>::STAT;

}

#endif /* DIALOG_DIALOG_MEMPOOL_H_ */

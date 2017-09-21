#ifndef DIALOG_DIALOG_MEMPOOL_H_
#define DIALOG_DIALOG_MEMPOOL_H_

#include <typeinfo>
#include "atomic.h"
#include "configuration_params.h"
#include "mempool_stat.h"

namespace dialog {

template<typename T>
class mempool {

 public:
  /**
   * Allocate memory for array_size instances of T
   * @param array_size size of array to allocate
   * @return pointer to allocated memory
   */
  T* alloc(size_t array_size = 1) {
    if (STAT.get() >= configuration_params::MAX_MEMORY) {
      THROW(mempool_exception, "Max memory reached!");
    }
    T* ptr = new T[array_size];
    STAT.increment(array_size * sizeof(T));
    return ptr;
  }

  /**
   * Deallocate memory allocated by the mempool
   * @param ptr pointer to allocated memory
   * @param array_size size of allocated array
   */
  void dealloc(T* ptr, size_t array_size = 1) {
    delete[] ptr;
    STAT.decrement(array_size * sizeof(T));
  }

 private:
  static mempool_stat STAT;

};

template<typename T>
mempool_stat mempool<T>::STAT;

}

#endif /* DIALOG_DIALOG_MEMPOOL_H_ */

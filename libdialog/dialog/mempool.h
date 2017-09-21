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
    max_memory_(configuration_params::MAX_MEMORY())
    {
  }

  /**
   * Allocate memory for array_size instances of T
   * @param array_size size of array to allocate
   * @return pointer to allocated memory
   */
  T* alloc(int array_size = BLOCK_ARRAY_SIZE) {
    if (STAT.get() >= max_memory_) {
      THROW(mempool_exception, "Max memory reached!");
    }
    size_t num_blocks = 1 + (array_size - 1) / BLOCK_ARRAY_SIZE;
    T* ptr = new T[num_blocks * BLOCK_ARRAY_SIZE];
    STAT.increment(num_blocks * BLOCK_SIZE);
    return ptr;
  }

  /**
   * Deallocate memory allocated by the mempool
   * @param ptr pointer to allocated memory
   * @param array_size size of allocated array
   */
  void dealloc(T* ptr, int array_size = BLOCK_ARRAY_SIZE) {
    delete[] ptr;
    size_t num_blocks = 1 + (array_size - 1) / BLOCK_ARRAY_SIZE;
    STAT.decrement(num_blocks * BLOCK_SIZE);
  }

 private:
  static mempool_stat STAT;
  static const int BLOCK_ARRAY_SIZE;
  const size_t max_memory_;

};


template<typename T, size_t BLOCK_SIZE>
const int mempool<T, BLOCK_SIZE>::BLOCK_ARRAY_SIZE = BLOCK_SIZE * sizeof(T);

template<typename T, size_t BLOCK_SIZE>
mempool_stat mempool<T, BLOCK_SIZE>::STAT;

}

#endif /* DIALOG_DIALOG_MEMPOOL_H_ */

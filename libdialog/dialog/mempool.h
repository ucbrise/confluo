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
  mempool(bool prnt = false) :
    array_size_(BLOCK_SIZE / sizeof(T)),
    type_is_pointer_(std::is_pointer<T>::value),
    max_memory_(configuration_params::MAX_MEMORY)
    {
  }

  T* alloc() {
    if (STAT.get() >= max_memory_) {
      THROW(mempool_exception, "Max memory reached!");
    }
    int64_t b = utils::time_utils::cur_ns();
    T* ptr = new T[array_size_]();
    int64_t c = utils::time_utils::cur_ns();
    STAT.increment(BLOCK_SIZE);
    return ptr;
  }

  template<typename ... ARGS>
  T* alloc(ARGS&& ... args) {
    if (STAT.get() >= max_memory_) {
      THROW(mempool_exception, "Max memory reached!");
    }
    T* ptr = new T[array_size_];
    for (int i = 0; i < array_size_; i++) {
      new (&ptr[i]) T(std::forward<ARGS>(args)...);
    }
    STAT.increment(BLOCK_SIZE);
    return ptr;
  }

  void dealloc(T* ptr) {
//    TODO: don't call destructor for primitives
//    (the below attempt still doesn't compile)
//    if (type_is_pointer_) {
//      for (int i = 0; i < array_size_; i++) {
//        ptr[i]->~T();
//      }
//    }
    delete[] ptr;
    STAT.decrement(BLOCK_SIZE);
  }

 private:
  static mempool_stat STAT;
  const int array_size_;
  const bool type_is_pointer_;
  const size_t max_memory_;

};

template<typename T, size_t BLOCK_SIZE>
mempool_stat mempool<T, BLOCK_SIZE>::STAT;

}

#endif /* DIALOG_DIALOG_MEMPOOL_H_ */

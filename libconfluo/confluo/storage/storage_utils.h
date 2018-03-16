#ifndef CONFLUO_STORAGE_STORAGE_UTILS_H_
#define CONFLUO_STORAGE_STORAGE_UTILS_H_

#include <new>
#include "ptr_metadata.h"

namespace confluo {
namespace storage {

template<class T, class enabler = void>
struct lifecycle_util {
  /**
   * Construction using placement new.
   * @param ptr pointer to array allocated by an allocator
   */
  static void construct(void* ptr) {
    size_t len = ptr_metadata::get(ptr)->data_size_ / sizeof(T);
    T* casted_ptr = reinterpret_cast<T*>(ptr);
    for (size_t i = 0; i < len; i++) {
      new (casted_ptr + i) T();
    }
  }

  /**
   * Utility to call destructor explicitly for pointer.
   * This is useful for allocations done using placement new
   * since delete[] would not call the destructor.
   * @param ptr pointer to array allocated by an allocator
   */
  static void destroy(void* ptr) {
    size_t len = ptr_metadata::get(ptr)->data_size_ / sizeof(T);
    T* casted_ptr = reinterpret_cast<T*>(ptr);
    for (size_t i = 0; i < len; i++) {
      casted_ptr[i].~T();
    }
  }
};

/**
 * Specialization does nothing for fundamental types.
 */
template<class T>
struct lifecycle_util<T, std::enable_if<std::is_fundamental<T>::value>> {
  static void destroy(void* ptr) { }
};

}
}

#endif /* CONFLUO_STORAGE_STORAGE_UTILS_H_ */

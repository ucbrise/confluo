#ifndef CONFLUO_STORAGE_ENCODED_PTR_H_
#define CONFLUO_STORAGE_ENCODED_PTR_H_

#include "ptr_metadata.h"

namespace confluo {
namespace storage {

// TODO split into interface and 2 implementations, one for in-memory and one for archived representations.
template<typename T>
class encoded_ptr {
 public:
  typedef std::unique_ptr<T, void (*)(T*)> decoded_ptr;

  encoded_ptr(void* ptr = nullptr, size_t offset = 0)
      : ptr_(ptr) {
  }

  /**
   * @return internal encoded pointer
   */
  void* internal_ptr() const {
    return ptr_;
  }

  // Encode/decode member functions

  void encode(size_t idx, T val) {
    static_cast<T*>(ptr_)[idx] = val;
  }

  void encode(size_t idx, const T* data, size_t len) {
    memcpy(&static_cast<T*>(ptr_)[idx], data, sizeof(T) * len);
  }

  T decode(size_t idx) const {
    return static_cast<T*>(ptr_)[idx];
  }

  void decode(T* buffer, size_t idx, size_t len) const {
    memcpy(buffer, &static_cast<T*>(ptr_)[idx], sizeof(T) * len);
  }

  decoded_ptr decode_ptr(size_t idx = 0) const {
    T* ptr = ptr_ == nullptr ? nullptr : static_cast<T*>(ptr_) + idx;
    return decoded_ptr(ptr, no_op_delete);
  }

 private:
  /**
   * No-op.
   * @param ptr pointer
   */
  static void no_op_delete(T* ptr) { }

  void* ptr_; // internal encoded pointer

};

}
}

#endif /* CONFLUO_STORAGE_ENCODED_PTR_H_ */

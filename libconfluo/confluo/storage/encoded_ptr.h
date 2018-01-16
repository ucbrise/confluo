#ifndef CONFLUO_STORAGE_ENCODED_PTR_H_
#define CONFLUO_STORAGE_ENCODED_PTR_H_

#include "ptr_metadata.h"

namespace confluo {
namespace storage {

// TODO may need to inherit from unique_ptr so we can provide a default
//  constructor in order to make things cleaner for the caller.
template <typename T>
using decoded_ptr = typename std::unique_ptr<T, void (*)(T*)>;

template<typename T>
class encoded_ptr {
 public:
  encoded_ptr(void* ptr = nullptr, size_t offset = 0)
      : ptr_(ptr) {
  }

  /**
   * @return encoded pointer
   */
  void* ptr() const {
    return ptr_;
  }

  /**
   * @return encoded pointer
   */
  template<typename U>
  U* ptr_as() const {
    return static_cast<U*>(ptr_);
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

  decoded_ptr<T> decode_ptr(size_t idx = 0) const {
    T* ptr = ptr_ == nullptr ? nullptr : static_cast<T*>(ptr_) + idx;
    return decoded_ptr<T>(ptr, no_op_delete);
  }

 private:
  /**
   * No-op.
   * @param ptr pointer
   */
  static void no_op_delete(T* ptr) { }

  void* ptr_;

};

}
}

#endif /* CONFLUO_STORAGE_ENCODED_PTR_H_ */

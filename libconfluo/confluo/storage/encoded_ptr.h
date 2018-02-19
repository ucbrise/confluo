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
    ptr_aux_block aux = ptr_aux_block::get(ptr_metadata::get(ptr_));
    switch (aux.encoding_) {
      case encoding_type::D_UNENCODED: {
        static_cast<T*>(ptr_)[idx] = val;
        break;
      }
      default: {
        THROW(unsupported_exception, "Writing to an encoded pointer is unsupported!");
      }
    }
  }

  void encode(size_t idx, const T* data, size_t len) {
    ptr_aux_block aux = ptr_aux_block::get(ptr_metadata::get(ptr_));
    switch (aux.encoding_) {
      case encoding_type::D_UNENCODED: {
        memcpy(&static_cast<T*>(ptr_)[idx], data, sizeof(T) * len);
        break;
      }
      default: {
        THROW(illegal_state_exception, "Writing to an encoded pointer is unsupported!");
      }
    }
  }

  T decode(size_t idx) const {
    ptr_aux_block aux = ptr_aux_block::get(ptr_metadata::get(ptr_));
    switch (aux.encoding_) {
      case encoding_type::D_UNENCODED: {
        return static_cast<T*>(ptr_)[idx];
      }
      default: {
        THROW(illegal_state_exception, "Invalid encoding type!");
      }
    }
  }

  void decode(T* buffer, size_t idx, size_t len) const {
    ptr_aux_block aux = ptr_aux_block::get(ptr_metadata::get(ptr_));
    switch (aux.encoding_) {
      case encoding_type::D_UNENCODED: {
        memcpy(buffer, &static_cast<T*>(ptr_)[idx], sizeof(T) * len);
        break;
      }
      default: {
        THROW(illegal_state_exception, "Invalid encoding type!");
      }
    }
  }

  /**
   * Decode entire pointer, starting at index idx.
   * @param idx index
   * @return decoded pointer
   */
  decoded_ptr<T> decode_ptr(size_t idx = 0) const {
    ptr_aux_block aux = ptr_aux_block::get(ptr_metadata::get(ptr_));
    switch (aux.encoding_) {
      case encoding_type::D_UNENCODED: {
        T* ptr = ptr_ == nullptr ? nullptr : static_cast<T*>(ptr_) + idx;
        return decoded_ptr<T>(ptr, no_op_delete);
      }
      default: {
        THROW(illegal_state_exception, "Invalid encoding type!");
      }
    }
  }

 private:
  /**
   * No-op.
   * @param ptr pointer
   */
  static void no_op_delete(T* ptr) { }

  void* ptr_; // encoded data stored at this pointer

};

}
}

#endif /* CONFLUO_STORAGE_ENCODED_PTR_H_ */

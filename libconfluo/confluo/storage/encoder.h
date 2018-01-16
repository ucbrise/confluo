#ifndef CONFLUO_STORAGE_ENCODER_H_
#define CONFLUO_STORAGE_ENCODER_H_

#include "ptr_metadata.h"

namespace confluo {
namespace storage {

enum encoding_type {
  IDENTITY
};

class data_ptr {

 public:
  typedef std::unique_ptr<uint8_t, void (*)(uint8_t*)> simple_ptr;

  data_ptr(simple_ptr ptr, size_t size)
      : ptr_(std::move(ptr)),
        size_(size) {
  }

  uint8_t* get() {
    return ptr_.get();
  }

  size_t size() {
    return size_;
  }

 private:
  simple_ptr ptr_;
  size_t size_;

};

class encoder {
 public:

  /**
   * Encode pointer. Currently an identity operation.
   * @param ptr unencoded data pointer, allocated by the storage allocator.
   * @return pointer to raw encoded data (no metadata)
   */
  template<typename T, encoding_type ENCODING>
  static data_ptr encode(T* ptr) {
    size_t encoded_size = storage::ptr_metadata::get(ptr)->data_size_;
    return data_ptr(data_ptr::simple_ptr(reinterpret_cast<uint8_t*>(ptr), no_op_delete), encoded_size);
  }

 private:
  static void no_op_delete(uint8_t* ptr) { }

};

}
}

#endif /* CONFLUO_STORAGE_ENCODER_H_ */

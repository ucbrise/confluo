#ifndef CONFLUO_STORAGE_UNIQUE_BYTE_ARRAY_H_
#define CONFLUO_STORAGE_UNIQUE_BYTE_ARRAY_H_

#include <memory>

namespace confluo {

class unique_byte_array {

 public:
  typedef void (*deleter_fn) (uint8_t* data_ptr);
  typedef std::unique_ptr<uint8_t, void (*)(uint8_t*)> unique_uint8_t_ptr;

  unique_byte_array(uint8_t* data, size_t size, deleter_fn deleter = array_delete)
      : ptr_(data, deleter),
        size_(size) {
  }

  unique_byte_array(size_t size, deleter_fn deleter = array_delete)
      : unique_byte_array(new uint8_t[size], size, deleter) {
  }

  uint8_t* get() {
    return ptr_.get();
  }

  size_t size() {
    return size_;
  }

 private:
  static void array_delete(uint8_t* ptr) { delete[] ptr; }

  unique_uint8_t_ptr ptr_;
  size_t size_;

};

}

#endif /* CONFLUO_CONTAINER_UNIQUE_BYTE_ARRAY_H_ */

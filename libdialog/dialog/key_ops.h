#ifndef DIALOG_KEY_OPS_H_
#define DIALOG_KEY_OPS_H_

#include <type_traits>
#include "byte_string.h"

namespace dialog {

typedef byte_string (*key_op)(const void* data, double bucket_size);

template<typename T>
byte_string key_transform(const void* data, double bucket_size) {
  return byte_string(
      static_cast<T>(*reinterpret_cast<const T*>(data) / bucket_size));
}

template<>
byte_string key_transform<float>(const void* data, double bucket_size) {
  return byte_string(
      static_cast<int32_t>(*reinterpret_cast<const float*>(data) / bucket_size));
}

template<>
byte_string key_transform<double>(const void* data, double bucket_size) {
  return byte_string(
      static_cast<int32_t>(*reinterpret_cast<const double*>(data) / bucket_size));
}

template<>
byte_string key_transform<std::string>(const void* data, double bucket_size) {
  throw unsupported_exception("key_transform not supported for string type");
}

template<>
byte_string key_transform<void>(const void* data, double bucket_size) {
  throw unsupported_exception("key_transform not supported for none type");
}

}

#endif /* DIALOG_KEY_OPS_H_ */

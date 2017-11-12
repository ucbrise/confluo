#ifndef DIALOG_KEY_OPS_H_
#define DIALOG_KEY_OPS_H_

#include <type_traits>

#include "types/data.h"
#include "types/byte_string.h"

namespace dialog {

typedef byte_string (*key_op)(const data& v, double bucket_size);

template<typename T>
byte_string key_transform(const data& v, double bucket_size) {
  return byte_string(static_cast<T>(v.as<T>() / bucket_size));
}

template<>
byte_string key_transform<float>(const data& v, double bucket_size) {
  float val = v.as<float>() / bucket_size;
  if (val < std::numeric_limits<int32_t>::min()) {
    return byte_string(std::numeric_limits<int32_t>::min());
  } else if (val > std::numeric_limits<int32_t>::max()) {
    return byte_string(std::numeric_limits<int32_t>::max());
  }
  return byte_string(static_cast<int32_t>(val));
}

template<>
byte_string key_transform<double>(const data& v, double bucket_size) {
  double val = v.as<double>() / bucket_size;
  if (val < std::numeric_limits<int64_t>::min()) {
    return byte_string(std::numeric_limits<int64_t>::min());
  } else if (val > std::numeric_limits<int64_t>::max()) {
    return byte_string(std::numeric_limits<int64_t>::max());
  }
  return byte_string(static_cast<int64_t>(val));
}

template<>
byte_string key_transform<std::string>(const data& v, double bucket_size) {
  return byte_string(v.as<std::string>());
}

template<>
byte_string key_transform<void>(const data& data, double bucket_size) {
  THROW(unsupported_exception, "key_transform not supported for none type");
}

}

#endif /* DIALOG_KEY_OPS_H_ */

#ifndef CONFLUO_TYPES_KEY_OPS_H_
#define CONFLUO_TYPES_KEY_OPS_H_

#include <type_traits>

#include "raw_data.h"
#include "byte_string.h"

namespace confluo {

/** A function that maps raw immutable data into a byte string used for
 * lookup 
 */
typedef byte_string (*key_op_t)(const immutable_raw_data &v, double bucket_size);

/**
 * Converts raw immutable data into a byte string based on bucket size for
 * lookup
 *
 * @tparam T The type of data the raw immutable value is
 * @param v The raw immutable data that is transformed
 * @param bucket_size The bucket_size used for indexing
 *
 * @return The byte string 
 */
template<typename T>
inline byte_string key_transform(const immutable_raw_data &v, double bucket_size) {
  return byte_string(static_cast<T>(v.as<T>() / static_cast<T>(bucket_size)));
}

/**
 * Converts raw immutable data into a byte string interpreting the raw data
 * as a single precision floating point number
 *
 * @param v The raw immutable data used for transformation
 * @param bucket_size The bucket_size used for indexing
 *
 * @return The byte string associated with the float immutable value
 */
template<>
inline byte_string key_transform<float>(const immutable_raw_data &v, double bucket_size) {
  float val = static_cast<float>(v.as<float>() / bucket_size);
  if (val < std::numeric_limits<int32_t>::min()) {
    return byte_string(std::numeric_limits<int32_t>::min());
  } else if (val > std::numeric_limits<int32_t>::max()) {
    return byte_string(std::numeric_limits<int32_t>::max());
  }
  return byte_string(static_cast<int32_t>(val));
}

/**
 * Converts raw immutable data into a byte string interpreting the raw data
 * as a double
 *
 * @param v The raw immutable data to transform
 * @param bucket_size The bucket_size used for indexing
 *
 * @return The byte string associated with the double immutable value
 */
template<>
inline byte_string key_transform<double>(const immutable_raw_data &v, double bucket_size) {
  double val = v.as<double>() / bucket_size;
  if (val < std::numeric_limits<int64_t>::min()) {
    return byte_string(std::numeric_limits<int64_t>::min());
  } else if (val > std::numeric_limits<int64_t>::max()) {
    return byte_string(std::numeric_limits<int64_t>::max());
  }
  return byte_string(static_cast<int64_t>(val));
}

/**
 * Transforms raw immutable data into a byte string for lookup for string
 * immutable values
 *
 * @param v The raw immutable data used for transformation
 * @param bucket_size The bucket_size used for indexing
 *
 * @return The byte string associated with interpreting the raw immutable
 * data as a string
 */
template<>
inline byte_string key_transform<std::string>(const immutable_raw_data &v, double) {
  return byte_string(v.as<std::string>());
}

/**
 * Key transformation for void types
 *
 * @param data The raw immutable data to transform
 * @param bucket_size The bucket_size used for indexing
 *
 *
 * @throw unsupported_exception This operation is not supported for void
 * types
 */
template<>
inline byte_string key_transform<void>(const immutable_raw_data &data, double) {
  THROW(unsupported_exception, "key_transform not supported for none type");
}

}

#endif /* CONFLUO_TYPES_KEY_OPS_H_ */

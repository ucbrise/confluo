#ifndef CONFLUO_TYPES_SERDE_OPS_H_
#define CONFLUO_TYPES_SERDE_OPS_H_

#include "raw_data.h"

namespace confluo {

typedef void (*serialize_op_t)(std::ostream&, const immutable_raw_data&);
typedef void (*deserialize_op_t)(std::istream&, mutable_raw_data&);

/**
 * Serializes the raw immutable data to the specified output stream
 *
 * @tparam T The type of data the immutable raw data contains
 * @param out The output stream to write to
 * @param value The immutable raw data to serialize
 */
template<typename T>
static void serialize(std::ostream& out, const immutable_raw_data& value) {
  out.write(reinterpret_cast<const char*>(value.ptr), value.size);
}

/**
 * Serializes the raw immutable data to the specified output stream, 
 * for the void type
 *
 * @param out The output stream to write to
 * @param value The immutable raw data to serialize
 * @throw unsupported_exception This operation is not defined for void
 * type
 */
template<>
void serialize<void>(std::ostream& out, const immutable_raw_data& value) {
  THROW(unsupported_exception, "Serialize not supported for none type");
}

/**
 * Deserializes the data from the input stream to the given mutable
 * raw data value
 *
 * @tparam T The type of data the mutable raw data contains
 * @param in The input stream to read from
 * @param value The mutable raw data to contain the data
 */
template<typename T>
static void deserialize(std::istream& in, mutable_raw_data& out) {
  in.read(reinterpret_cast<char*>(out.ptr), out.size);
}

/**
 * Deserializes the data from the input stream to the given mutable
 * raw data value, for the void type
 *
 * @param in The input stream to read from
 * @param value The mutable raw data to contain the data
 * @throw unsupported_exception This operation is not defined for the 
 * void type
 */
template<>
void deserialize<void>(std::istream& in, mutable_raw_data& value) {
  THROW(unsupported_exception, "Deserialize not supported for none type");
}

}

#endif /* CONFLUO_TYPES_SERDE_OPS_H_ */

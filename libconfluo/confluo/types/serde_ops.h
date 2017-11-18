#ifndef CONFLUO_TYPES_SERDE_OPS_H_
#define CONFLUO_TYPES_SERDE_OPS_H_

#include "raw_data.h"

namespace confluo {

typedef void (*serialize_op_t)(std::ostream&, const immutable_raw_data&);
typedef void (*deserialize_op_t)(std::istream&, mutable_raw_data&);

template<typename T>
static void serialize(std::ostream& out, const immutable_raw_data& value) {
  out.write(reinterpret_cast<const char*>(value.ptr), value.size);
}

template<>
void serialize<void>(std::ostream& out, const immutable_raw_data& value) {
  THROW(unsupported_exception, "Serialize not supported for none type");
}

template<typename T>
static void deserialize(std::istream& in, mutable_raw_data& out) {
  in.read(reinterpret_cast<char*>(out.ptr), out.size);
}

template<>
void deserialize<void>(std::istream& in, mutable_raw_data& value) {
  THROW(unsupported_exception, "Deserialize not supported for none type");
}

}

#endif /* CONFLUO_TYPES_SERDE_OPS_H_ */

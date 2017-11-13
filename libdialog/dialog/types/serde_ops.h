#ifndef DIALOG_SERDE_OPS_H_
#define DIALOG_SERDE_OPS_H_

#include "data.h"

namespace dialog {

typedef void (*serialize_op_t)(std::ostream&, const data&);
typedef void (*deserialize_op_t)(std::istream&, data&);

template<typename T>
static void serialize(std::ostream& out, const data& value) {
  out.write(reinterpret_cast<const char*>(value.ptr), value.size);
}

template<>
void serialize<void>(std::ostream& out, const data& value) {
  THROW(unsupported_exception, "Serialize not supported for none type");
}

template<>
void serialize<std::string>(std::ostream& out, const data& value) {
  out.write(reinterpret_cast<const char*>(value.size), sizeof(size_t));
  out.write(reinterpret_cast<const char*>(value.ptr), value.size);
}

template<typename T>
static void deserialize(std::istream& in, data& out) {
  in.read(reinterpret_cast<char*>(const_cast<void*>(out.ptr)), out.size);
}

template<>
void deserialize<void>(std::istream& in, data& value) {
  THROW(unsupported_exception, "Deserialize not supported for none type");
}

template<>
void deserialize<std::string>(std::istream& in, data& value) {
  in.read(reinterpret_cast<char*>(value.size), sizeof(size_t));
  char *buf = new char[value.size];
  in.read(buf, value.size);
  value.ptr = buf;
}

}

#endif /* DIALOG_SERDE_OPS_H_ */

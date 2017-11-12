#ifndef DIALOG_SERDE_OPS_H_
#define DIALOG_SERDE_OPS_H_

#include "data.h"

namespace dialog {

typedef void (*serialize_op)(std::ostream&, data&);
typedef void (*deserialize_op)(std::istream&, data&);

template<typename T>
static void serialize(std::ostream& out, data& value) {
  //T val = value.as<T>();
  char* val_char = (char*) value.ptr;
  out.write(val_char, value.size);
}

template<typename T>
static void deserialize(std::istream& in, data& out) {
  in.read(reinterpret_cast<char*>(const_cast<void*>(out.ptr)), out.size);
  //const void* const_ptr = const_cast<const void*>(val_ptr);
  //return out;
}

template<>
void serialize<std::string>(std::ostream& out, data& value) {

}

static std::vector<void (*)(std::ostream&, data&)> init_serializers() {
  return {&serialize<bool>, &serialize<bool>, &serialize<char>,
    &serialize<short>, &serialize<int>, &serialize<long>,
    &serialize<float>, &serialize<double>, &serialize<std::string>};
}

static std::vector<void (*)(std::istream&, data&)> init_deserializers() {
  return {&deserialize<bool>, &deserialize<bool>, &deserialize<char>,
    &deserialize<short>, &deserialize<int>, &deserialize<long>,
    &deserialize<float>, &deserialize<double>, &deserialize<std::string>};
}

}

#endif /* DIALOG_SERDE_OPS_H_ */

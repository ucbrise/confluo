#ifndef DIALOG_TYPE_PROPERTIES_H_
#define DIALOG_TYPE_PROPERTIES_H_

#include "arithmetic_ops.h"
#include "relational_ops.h"
#include "key_ops.h"
#include "string_ops.h"
#include "serde_ops.h"
#include "primitive_types.h"

namespace dialog {

struct type_properties {
  std::string name;
  size_t size;

  void* min;
  void* max;
  void* one;
  void* zero;

  rel_ops_t rel_ops;
  unary_ops_t un_ops;
  binary_ops_t binary_ops;
  key_op key_ops;

  data (*parse_op)(const std::string&);

  void (*serialize_op)(std::ostream&, data&);
  void (*deserialize_op)(std::istream&, data&);

  type_properties(const std::string& _name, size_t _size, void* _min,
                  void* _max, void* _one, void* _zero, rel_ops_t _rel_ops,
                  unary_ops_t _un_ops, binary_ops_t _binary_ops,
                  key_op _key_ops, data (*_parse)(const std::string&),
                  void (*_serialize)(std::ostream&, data&),
                  void (*_deserialize)(std::istream&, data&))
      : name(_name),
        size(_size),
        min(_min),
        max(_max),
        one(_one),
        zero(_zero),
        rel_ops(_rel_ops),
        un_ops(_un_ops),
        binary_ops(_binary_ops),
        key_ops(_key_ops),
        parse_op(_parse),
        serialize_op(_serialize),
        deserialize_op(_deserialize) {
  }
};

template<typename T>
type_properties build_properties(const std::string& name, size_t size =
                                     sizeof(T),
                                 void* min = nullptr, void* max = nullptr,
                                 void* one = nullptr, void* zero = nullptr) {
  return type_properties(name, size, min, max, one, zero, init_relops<T>(),
                         init_unaryops<T>(), init_binaryops<T>(),
                         key_transform<T>, parse<T>, serialize<T>,
                         deserialize<T>);
}

#define TMIN(name) name ## _min
#define TMAX(name) name ## _max
#define TONE(name) name ## _one
#define TZERO(name) name ## _zero
#define DEFINE_PRIMITIVE(TNAME, T)\
    build_properties<T>(#TNAME, sizeof(T), &limits::TMIN(TNAME), &limits::TMAX(TNAME),\
                     &limits::TONE(TNAME), &limits::TZERO(TNAME))

namespace detail {

std::vector<type_properties> init_primitives() {
  std::vector<type_properties> props;
  props.push_back(build_properties<void>("none", 0));
  props.push_back(DEFINE_PRIMITIVE(bool, bool));
  props.push_back(DEFINE_PRIMITIVE(char, int8_t));
  props.push_back(DEFINE_PRIMITIVE(short, int16_t));
  props.push_back(DEFINE_PRIMITIVE(int, int32_t));
  props.push_back(DEFINE_PRIMITIVE(long, int64_t));
  props.push_back(DEFINE_PRIMITIVE(float, float));
  props.push_back(DEFINE_PRIMITIVE(double, float));
  props.push_back(build_properties<std::string>("string", 0));
  return props;
}

}

std::vector<type_properties> DATA_TYPES = detail::init_primitives();

}

#endif /* DIALOG_TYPE_PROPERTIES_H_ */

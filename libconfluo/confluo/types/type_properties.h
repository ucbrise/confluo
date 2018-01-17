#ifndef CONFLUO_TYPES_TYPE_PROPERTIES_H_
#define CONFLUO_TYPES_TYPE_PROPERTIES_H_

#include "primitive_types.h"
#include "arithmetic_ops.h"
#include "key_ops.h"
#include "relational_ops.h"
#include "serde_ops.h"
#include "string_ops.h"
#include "string_utils.h"

namespace confluo {

struct type_properties {
  std::string name;
  size_t size;

  void* min;
  void* max;
  void* one;
  void* zero;

  bool is_numeric;

  rel_ops_t relational_ops;
  unary_ops_t unary_ops;
  binary_ops_t binary_ops;
  key_op_t key_transform_op;

  parse_op_t parse_op;
  to_string_op_t to_string_op;

  serialize_op_t serialize_op;
  deserialize_op_t deserialize_op;

  type_properties(const std::string& _name, size_t _size, void* _min,
                  void* _max, void* _one, void* _zero, bool _is_numeric,
                  rel_ops_t _rel_ops, unary_ops_t _un_ops,
                  binary_ops_t _binary_ops, key_op_t _key_ops,
                  parse_op_t _parse, to_string_op_t _to_string,
                  serialize_op_t _serialize, deserialize_op_t _deserialize)
      : name(_name),
        size(_size),
        min(_min),
        max(_max),
        one(_one),
        zero(_zero),
        is_numeric(_is_numeric),
        relational_ops(_rel_ops),
        unary_ops(_un_ops),
        binary_ops(_binary_ops),
        key_transform_op(_key_ops),
        parse_op(_parse),
        to_string_op(_to_string),
        serialize_op(_serialize),
        deserialize_op(_deserialize) {
  }
};

template<typename T>
type_properties build_properties(const std::string& name, size_t size =
                                     sizeof(T),
                                 bool is_numeric = true, void* min = nullptr,
                                 void* max = nullptr, void* one = nullptr,
                                 void* zero = nullptr) {
  return type_properties(name, size, min, max, one, zero, is_numeric,
                         init_relops<T>(), init_unaryops<T>(),
                         init_binaryops<T>(), key_transform<T>, parse<T>,
                         to_string<T>, serialize<T>, deserialize<T>);
}

namespace detail {

#define TMIN(name) name ## _min
#define TMAX(name) name ## _max
#define TONE(name) name ## _one
#define TZERO(name) name ## _zero
#define DEFINE_PRIMITIVE(TNAME, T)\
    build_properties<T>(#TNAME, sizeof(T), true, &limits::TMIN(TNAME),\
                        &limits::TMAX(TNAME), &limits::TONE(TNAME),\
                        &limits::TZERO(TNAME))

std::vector<type_properties> init_primitives() {
  std::vector<type_properties> props;
  props.push_back(build_properties<void>("none", 0, false));
  props.push_back(DEFINE_PRIMITIVE(bool, bool));
  props.push_back(DEFINE_PRIMITIVE(char, int8_t));
  props.push_back(DEFINE_PRIMITIVE(uchar, uint8_t));
  props.push_back(DEFINE_PRIMITIVE(short, int16_t));
  props.push_back(DEFINE_PRIMITIVE(ushort, uint16_t));
  props.push_back(DEFINE_PRIMITIVE(int, int32_t));
  props.push_back(DEFINE_PRIMITIVE(uint, uint32_t));
  props.push_back(DEFINE_PRIMITIVE(long, int64_t));
  props.push_back(DEFINE_PRIMITIVE(ulong, uint64_t));
  props.push_back(DEFINE_PRIMITIVE(float, float));
  props.push_back(DEFINE_PRIMITIVE(double, double));
  props.push_back(build_properties<std::string>("string", 0, false));
  return props;
}

}

std::vector<type_properties> DATA_TYPES = detail::init_primitives();

static size_t find_type_properties(const std::string& name) {
  std::string uname = utils::string_utils::to_upper(name);
  for (unsigned int i = 0; i < DATA_TYPES.size(); i++) {
    std::string tname = utils::string_utils::to_upper(DATA_TYPES[i].name);
    if (uname.compare(tname) == 0) {
      return i;
    }
  }
  return 0;
}

}

#endif /* CONFLUO_TYPES_TYPE_PROPERTIES_H_ */

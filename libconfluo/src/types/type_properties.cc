#include "types/type_properties.h"

namespace confluo {

type_properties::type_properties(const std::string &_name,
                                 size_t _size,
                                 void *_min,
                                 void *_max,
                                 void *_one,
                                 void *_zero,
                                 bool _is_numeric,
                                 rel_ops_t _rel_ops,
                                 unary_ops_t _un_ops,
                                 binary_ops_t _binary_ops,
                                 key_op_t _key_ops,
                                 parse_op_t _parse,
                                 to_string_op_t _to_string,
                                 serialize_op_t _serialize,
                                 deserialize_op_t _deserialize)
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

template<typename T>
type_properties build_properties(const std::string &name,
                                 size_t size,
                                 bool is_numeric,
                                 void *min,
                                 void *max,
                                 void *one,
                                 void *zero) {
  return type_properties(name, size, min, max, one, zero, is_numeric, init_relops<T>(), init_unaryops<T>(),
                         init_binaryops<T>(), key_transform<T>, parse<T>, to_string<T>, serialize<T>, deserialize<T>);
}

std::vector<type_properties> detail::init_primitives() {
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
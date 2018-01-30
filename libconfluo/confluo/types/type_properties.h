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

/**
 * Operators and values a user-defined data type needs to define
 */
struct type_properties {
  /** A unique name for the type */
  std::string name;
  /** The size of underlying representation for fixed sized types. 
   * This should be set to zero for dynamically sized types */
  size_t size;

  /** This is a pointer to the minimum value that the type can hold */
  void* min;
  /** This is a pointer to the maximum value that the type can hold */
  void* max;
  /** This is a pointer to the step value with which the type can be
   * incremented. */
  void* one;
  /** This is a pointer to the zero value for the type. */
  void* zero;

  /** This indicates whether the type is numeric or not; numeric types 
   * typically support most arithmetic operators */
  bool is_numeric;

  /** Stores a list of relational operator functions for the given type, i
   * so that operations like filter can work. */
  rel_ops_t relational_ops;
  /** Stores a list of unary arithmetic operator functions for the given 
   * type, so that operations like filter can work for the given type. */
  unary_ops_t unary_ops;
  /** Stores a list of binary arithmetic operator functions for the given 
   * type, so that operations like filter can accurately be applied to the 
   * type. */
  binary_ops_t binary_ops;
  /** Stores the key-transform function. This function is important for i
   * looking up attributes of the type in an index */
  key_op_t key_transform_op;

  /** Parses data instance from a string representation of this type */
  parse_op_t parse_op;
  /** Converts data instance of the type to its string representation. */
  to_string_op_t to_string_op;

  /** Serializes the underlying data representation of the type into 
   * raw bytes */
  serialize_op_t serialize_op;
  /** Reads the raw byte representation of the type and parses it to data */
  deserialize_op_t deserialize_op;

  /**
   * Constructs the properties of the type based on the specified functions
   * and values
   *
   * @param _name The name of the type
   * @param _size The size of the type
   * @param _min The min of the type
   * @param _max The max of the type
   * @param _one The one value of the type
   * @param _zero The zero value of the type
   * @param _is_numeric Whether the type is numeric
   * @param _rel_ops The relational operators defined for the type
   * @param _un_ops The unary operators defined for the type
   * @param _binary_ops The binary operators defined for the type
   * @param _key_ops The key operators defined for the type
   * @param _parse The parse function for the type
   * @param _to_string The to_string function for the data type
   * @param _serialize The serialize function for the data type
   * @param _deserialize The deserialize function for the data type
   */
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

/**
 * Constructs the type properties for the given type
 *
 * @tparam T The type to construct the properties for
 * @param name The name of the type
 * @param size The size of the type
 * @param is_numeric Whether the type is numeric
 * @param min The min of the type
 * @param max The max of the type
 * @param one The one value of the type
 * @param zero The zero value of the type
 *
 * @return The type properties for the type
 */
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

/**
 * Initializes the primitive data type properties for usage
 *
 * @return Vector of data type properties for the primitive data types
 */
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

/** The vector of data types */
std::vector<type_properties> DATA_TYPES = detail::init_primitives();

/**
 * Finds the type properties for the type specified by name
 *
 * @param name The name of the data type
 *
 * @return The index in the DATA_TYPES array to find the specified
 * type properties
 */
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

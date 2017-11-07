#ifndef DIALOG_DATA_TYPES_H_
#define DIALOG_DATA_TYPES_H_

#include <limits>
#include <string>
#include <array>
#include <cstdint>
#include <regex>
#include <cstring>

#include "data.h"
#include "relational_ops.h"
#include "arithmetic_ops.h"
#include "key_ops.h"
#include "exceptions.h"
#include "string_utils.h"

namespace dialog {

enum type_id
  : uint16_t {
    D_NONE = 0,
  D_BOOL = 1,
  D_CHAR = 2,
  D_SHORT = 3,
  D_INT = 4,
  D_LONG = 5,
  D_FLOAT = 6,
  D_DOUBLE = 7,
  D_STRING = 8
};

std::string none_string() {
    return "none";
}

std::string bool_string() {
    return "bool";
}

std::string char_string() {
    return "char";
}

std::string short_string() {
    return "short";
}

std::string int_string() {
    return "int";
}

std::string long_string() {
    return "long";
}

std::string float_string() {
    return "float";
}

std::string double_string() {
    return "double";
}

std::string string_string() {
    return "string";
}


namespace limits {

static bool bool_min = std::numeric_limits<bool>::lowest();
static bool bool_zero = false;
static bool bool_one = true;
static bool bool_max = std::numeric_limits<bool>::max();

static int8_t char_min = std::numeric_limits<int8_t>::lowest();
static int8_t char_zero = static_cast<int8_t>(0);
static int8_t char_one = static_cast<int8_t>(1);
static int8_t char_max = std::numeric_limits<int8_t>::max();

static int16_t short_min = std::numeric_limits<int16_t>::lowest();
static int16_t short_zero = static_cast<int16_t>(0);
static int16_t short_one = static_cast<int16_t>(1);
static int16_t short_max = std::numeric_limits<int16_t>::max();

static int32_t int_min = std::numeric_limits<int32_t>::lowest();
static int32_t int_zero = static_cast<int32_t>(0);
static int32_t int_one = static_cast<int32_t>(1);
static int32_t int_max = std::numeric_limits<int32_t>::max();

static int64_t long_min = std::numeric_limits<int64_t>::lowest();
static int64_t long_zero = static_cast<int64_t>(0);
static int64_t long_one = static_cast<int64_t>(1);
static int64_t long_max = std::numeric_limits<int64_t>::max();

static uint64_t long_long_min = std::numeric_limits<uint64_t>::lowest();
static uint64_t long_long_zero = static_cast<uint64_t>(0);
static uint64_t long_long_one = static_cast<uint64_t>(1);
static uint64_t long_long_max = std::numeric_limits<uint64_t>::max();

static float float_min = std::numeric_limits<float>::lowest();
static float float_zero = static_cast<float>(0.0);
static float float_one = static_cast<float>(1.0);
static float float_max = std::numeric_limits<float>::max();

static double double_min = std::numeric_limits<double>::lowest();
static double double_zero = static_cast<double>(0.0);
static double double_one = static_cast<double>(1.0);
static double double_max = std::numeric_limits<double>::max();

}

//std::vector<data_type> dialog::type_manager::data_types;

static std::vector<void*> init_min() {
  return {nullptr, &limits::bool_min, &limits::char_min, &limits::short_min,
    &limits::int_min, &limits::long_min, &limits::float_min,
    &limits::double_min, nullptr};
}

static std::vector<void*> init_max() {
  return {nullptr, &limits::bool_max, &limits::char_max, &limits::short_max,
    &limits::int_max, &limits::long_max, &limits::float_max,
    &limits::double_max, nullptr};
}

static std::vector<void*> init_one() {
  return {nullptr, &limits::bool_one, &limits::char_one, &limits::short_one,
    &limits::int_one, &limits::long_one, &limits::float_one,
    &limits::double_one, nullptr};
}

static std::vector<void*> init_zero() {
  return {nullptr, &limits::bool_zero, &limits::char_zero, &limits::short_zero,
    &limits::int_zero, &limits::long_zero, &limits::float_zero,
    &limits::double_zero, nullptr};
}

static std::vector<rel_ops_t> init_rops() {
  return {init_relops<void>(), init_relops<bool>(), init_relops<int8_t>(),
    init_relops<int16_t>(), init_relops<int32_t>(), init_relops<int64_t>(),
    init_relops<float>(), init_relops<double>(), init_relops<std::string>()};
}

static std::vector<unary_ops_t> init_uops() {
  return {init_unaryops<void>(), init_unaryops<bool>(), init_unaryops<int8_t>(),
    init_unaryops<int16_t>(), init_unaryops<int32_t>(), init_unaryops<int64_t>(),
    init_unaryops<float>(), init_unaryops<double>(), init_unaryops<std::string>()};
}

static std::vector<binary_ops_t> init_bops() {
  return {init_binaryops<void>(), init_binaryops<bool>(), init_binaryops<int8_t>(),
    init_binaryops<int16_t>(), init_binaryops<int32_t>(), init_binaryops<int64_t>(),
    init_binaryops<float>(), init_binaryops<double>(), init_binaryops<std::string>()};
}

static std::vector<key_op> init_kops() {
  return {key_transform<void>, key_transform<bool>, key_transform<int8_t>,
    key_transform<int16_t>, key_transform<int32_t>, key_transform<int64_t>,
    key_transform<float>, key_transform<double>, key_transform<std::string>};
}

static std::vector<std::string (*)()> init_to_strings() {
    return {&none_string, &bool_string, &char_string, &short_string,
        &int_string, &long_string, &float_string, &double_string,
    &string_string};
}

static std::vector<size_t> primitive_sizes() {
    return {0, sizeof(bool), sizeof(int8_t), sizeof(int16_t), 
        sizeof(int32_t), sizeof(int64_t), sizeof(float), sizeof(double)};
}

data parse_void(const std::string &str) {
    return data((void*) new char[0], 0);
    
}

data parse_bool(const std::string& str) {
    bool val = string_utils::lexical_cast<bool>(str);
    //return mutable_value(val);
    return data((void*) new bool(val), sizeof(bool));
}


data parse_char(const std::string& str) {
    int8_t val = string_utils::lexical_cast<int8_t>(str);
    return data((void*) new int8_t(val), sizeof(int8_t));
}


data parse_short(const std::string& str) {
    int16_t val = string_utils::lexical_cast<int16_t>(str);
    return data((void*) new int16_t(val), sizeof(int16_t));
}


data parse_int(const std::string& str) {
    int32_t val = string_utils::lexical_cast<int32_t>(str);
    return data((void*) new int32_t(val), sizeof(int32_t));
}


data parse_long(const std::string& str) {
    int64_t val = string_utils::lexical_cast<int64_t>(str);
    return data((void*) new int64_t(val), sizeof(int64_t));
}


data parse_float(const std::string& str) {
    float val = string_utils::lexical_cast<float>(str);
    return data((void*) new float(val), sizeof(float));
}


data parse_double(const std::string& str) {
    double val = string_utils::lexical_cast<double>(str);
    return data((void*) new double(val), sizeof(double));
}


data parse_string(const std::string& str) {
    char* characters = new char[strlen(str.c_str()) + 1];
    strcpy(characters, str.c_str());
    return data((void*) characters, strlen(str.c_str()) + 1);
}

static std::vector<data (*)(const std::string&)> init_parsers() {
   return {&parse_void, &parse_bool, &parse_char, &parse_short, &parse_int, &parse_long, &parse_float, &parse_double, &parse_string};
}

template<typename T>
static void serialize(std::ostream& out, data& value) {
    T val = value.as<T>();
    out.write(reinterpret_cast<const char*>(&val), value.size);
}

template<typename T>
static data deserialize(std::istream& in) {
    T* val_ptr = new T;
    in.read(reinterpret_cast<char*>(val_ptr), sizeof(T));
    void* void_ptr = (void*) val_ptr;
    const void* const_ptr = const_cast<const void*>(void_ptr);
    return data(const_ptr, sizeof(T));
}

template<>
void serialize<std::string>(std::ostream& out, data& value) {
    
}

static std::vector<void (*)(std::ostream&, data&)> init_serializers() {
    return {&serialize<bool>, &serialize<bool>, &serialize<char>,
    &serialize<short>, &serialize<int>, &serialize<long>, 
    &serialize<float>, &serialize<double>, &serialize<std::string>};
}

static std::vector<data (*)(std::istream&)> init_deserializers() {
    return {&deserialize<bool>, &deserialize<bool>, &deserialize<char>,
    &deserialize<short>, &deserialize<int>, &deserialize<long>,
    &deserialize<float>, &deserialize<double>, &deserialize<std::string>};
}

static std::vector<void*> MIN = init_min();
static std::vector<void*> MAX = init_max();
static std::vector<void*> ONE = init_one();
static std::vector<void*> ZERO = init_zero();
static std::vector<rel_ops_t> RELOPS = init_rops();
static std::vector<unary_ops_t> UNOPS = init_uops();
static std::vector<binary_ops_t> BINOPS = init_bops();
static std::vector<key_op> KEYOPS = init_kops();
static std::vector<std::string (*)()> TO_STRINGS = init_to_strings();
static std::vector<size_t> PRIMITIVE_SIZES = primitive_sizes();
static std::vector<data (*)(const std::string&)> PARSERS = 
    init_parsers();
static std::vector<void (*)(std::ostream&, data&)> SERIALIZERS = 
                init_serializers();
static std::vector<data (*)(std::istream&)> DESERIALIZERS = 
                init_deserializers();

struct data_type {
 public:
  uint16_t id;
  size_t size;

  bool is_primitive() {
      return id >= 0 && id <= 8;
  }

  data_type(type_id _id = type_id::D_NONE, size_t _size = 0)
      : id(_id),
        size(_size) {
    uint16_t string_id = 8;

    if (!is_primitive()) {
        THROW(invalid_operation_exception,
              "Must specify length for non-primitive data types");
    } else if (id != string_id) {
        size = PRIMITIVE_SIZES[id];
    }
  }

  data_type(uint16_t _id, size_t _size) {
      id = _id;
      size = _size;
  }

  void* min() const {
    return MIN[id];
  }

  void* max() const {
    return MAX[id];
  }

  void* one() const {
    return ONE[id];
  }

  void* zero() const {
    return ZERO[id];
  }

  inline data_type& operator=(const data_type& other) {
    id = other.id;
    size = other.size;
    return *this;
  }

  inline bool operator==(const data_type& other) const {
    return id == other.id && size == other.size;
  }

  inline bool operator!=(const data_type& other) const {
    return id != other.id || size != other.size;
  }

  inline const relational_fn& relop(relop_id rid) const {
    return RELOPS[id][rid];
  }

  inline const unary_fn& unaryop(unaryop_id uid) const {
    return UNOPS[id][uid];
  }

  inline const binary_fn& binaryop(binaryop_id bid) const {
    return BINOPS[id][bid];
  }

  inline const key_op& keytransform() const {
    return KEYOPS[id];
  }

  inline bool is_primitive() const {
      return id >= 0 && id <= 8;
  }

  inline std::string to_string() const {
      return TO_STRINGS[id]();
  }

  /*inline mutable_value parse(const std::string& str) const {
      return PARSERS[id](str);
  }*/

  inline static data_type from_string(const std::string& str) {
    std::string tstr = utils::string_utils::to_upper(str);
    std::regex str_re("STRING\\(([0-9]+)\\)");
    std::smatch str_matches;
    if (tstr == "BOOL") {
      return data_type(1, sizeof(bool));
    } else if (tstr == "CHAR") {
      return data_type(2, sizeof(int8_t));
    } else if (tstr == "SHORT") {
      return data_type(3, sizeof(int16_t));
    } else if (tstr == "INT") {
      return data_type(4, sizeof(int32_t));
    } else if (tstr == "LONG") {
      return data_type(5, sizeof(int64_t));
    } else if (tstr == "FLOAT") {
      return data_type(6, sizeof(float));
    } else if (tstr == "DOUBLE") {
      return data_type(7, sizeof(double));
    } else if (std::regex_search(tstr, str_matches, str_re)) {
      return data_type(8, std::stoul(str_matches[1].str()));
    }
    return data_type(0, 0);
  }
};

/*static data_type NONE_TYPE(type_id::D_NONE);
static data_type BOOL_TYPE(type_id::D_BOOL);
static data_type CHAR_TYPE(type_id::D_CHAR);
static data_type SHORT_TYPE(type_id::D_SHORT);
static data_type INT_TYPE(type_id::D_INT);
static data_type LONG_TYPE(type_id::D_LONG);
static data_type FLOAT_TYPE(type_id::D_FLOAT);
static data_type DOUBLE_TYPE(type_id::D_DOUBLE);
static data_type STRING_TYPE(size_t size) {
  return data_type(type_id::D_STRING, size);
}*/

// type-ids 1-7 are numeric
static inline bool is_numeric(const data_type& type) {
  return type.id >= 1 && type.id <= 7;
}

}

#endif /* DIALOG_DATA_TYPES_H_ */

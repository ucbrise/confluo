#ifndef CONFLUO_TEST_SIZE_TYPE_H_
#define CONFLUO_TEST_SIZE_TYPE_H_

#include <cstdint>
#include <regex>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "types/data_type.h"
#include "exceptions.h"
#include "types/byte_string.h"
#include "types/immutable_value.h"
#include "types/type_properties.h"

namespace confluo {

class size_type {
 public:
  size_type() {
    bytes = 0;
  }

  size_type(uint64_t value) {
    bytes = value;
  }

  static std::string name() {
    return "size_type";
  }

  std::string to_string() const {
    return std::to_string(bytes) + "b";
  }

  static size_type from_string(const std::string& str) {
    std::regex re("(\\d+(?:\\.\\d+)?)\\s*([kmgtp]?b)");
    std::smatch str_matches;
    if (std::regex_search(str, str_matches, re)) {
      const int num_order = 6;
      std::string sizes[num_order] = { "b", "kb", "mb", "gb", "tb", "pb" };

      uint64_t value = utils::string_utils::lexical_cast<uint64_t>(
          str_matches.str(1));

      std::string type = str_matches.str(2);
      std::transform(type.begin(), type.end(), type.begin(), ::tolower);
      uint64_t byt = 0;

      for (unsigned int i = 0; i < num_order; i++) {
        if (type.compare(sizes[i]) == 0) {
          byt = value * pow(1024, i);
          break;
        }
      }

      return size_type(byt);
    }
    THROW(unsupported_exception, "invalid size string");
  }

  static void parse_bytes(const std::string& str, void* out) {
    size_type sz = size_type::from_string(str);
    memcpy(out, &sz, sizeof(size_type));
  }

  static std::string size_to_string(const immutable_raw_data& data) {
    return data.as<size_type>().to_string();
  }

  std::string repr() const {
    return std::to_string(bytes);
  }

  uint64_t get_bytes() const {
    return bytes;
  }
  void set_bytes(uint64_t _bytes) {
    bytes = _bytes;
  }

 private:
  uint64_t bytes;
}__attribute__((packed));

template<>
void serialize<size_type>(std::ostream& out, const immutable_raw_data& value) {
  size_type val = value.as<size_type>();
  uint64_t val_bytes = val.get_bytes();
  out.write(reinterpret_cast<const char*>(&(val_bytes)), value.size);
}

template<>
void deserialize<size_type>(std::istream& in, mutable_raw_data& out) {
  uint64_t val;
  in.read(reinterpret_cast<char*>(&out.ptr), sizeof(uint64_t));
}

template<>
inline void add<size_type>(void* res, const immutable_raw_data& v1,
                           const immutable_raw_data& v2) {
  (*(reinterpret_cast<size_type*>(res))).set_bytes(
      v1.as<size_type>().get_bytes() + v2.as<size_type>().get_bytes());
}

template<>
inline void subtract<size_type>(void* res, const immutable_raw_data& v1,
                                const immutable_raw_data& v2) {
  (*(reinterpret_cast<size_type*>(res))).set_bytes(
      v1.as<size_type>().get_bytes() - v2.as<size_type>().get_bytes());
}

template<>
inline void multiply<size_type>(void* res, const immutable_raw_data& v1,
                                const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 1");
}

template<>
inline void divide<size_type>(void* res, const immutable_raw_data& v1,
                              const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 2");
}

template<>
inline void modulo<size_type>(void* res, const immutable_raw_data& v1,
                              const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 3");
}

template<>
inline void bw_and<size_type>(void* res, const immutable_raw_data& v1,
                              const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 4");
}

template<>
inline void bw_or<size_type>(void* res, const immutable_raw_data& v1,
                             const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 5");
}

template<>
inline void bw_xor<size_type>(void* res, const immutable_raw_data& v1,
                              const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 6");
}

template<>
inline void bw_lshift<size_type>(void* res, const immutable_raw_data& v1,
                                 const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 7");
}

template<>
inline void bw_rshift<size_type>(void* res, const immutable_raw_data& v1,
                                 const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 8");
}

template<>
inline void assign<size_type>(void* res, const immutable_raw_data& v) {
  (*(reinterpret_cast<size_type*>(res))).set_bytes(
      v.as<size_type>().get_bytes());
}

template<>
inline void negative<size_type>(void* res, const immutable_raw_data& v) {
  (*(reinterpret_cast<size_type*>(res))).set_bytes(
      -v.as<size_type>().get_bytes());
}

template<>
inline void positive<size_type>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "operation not yet supported 10");
}

template<>
inline void bw_not<size_type>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "operation not yet supported 11");
}

template<>
inline bool less_than<size_type>(const immutable_raw_data& v1,
                                 const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 12");
}

template<>
inline bool less_than_equals<size_type>(const immutable_raw_data& v1,
                                        const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 13");
}

template<>
inline bool greater_than<size_type>(const immutable_raw_data& v1,
                                    const immutable_raw_data& v2) {
  return v1.as<size_type>().get_bytes() > v2.as<size_type>().get_bytes();
  //THROW(unsupported_exception, "operation not supported 69");
}

template<>
inline bool greater_than_equals<size_type>(const immutable_raw_data& v1,
                                           const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 14");
}

template<>
inline bool equals<size_type>(const immutable_raw_data& v1,
                              const immutable_raw_data& v2) {
  //std::cout << "v1: " << v1.as<size_type>().get_bytes() << std::endl;
  //std::cout << "v2: " << v2.as<size_type>().get_bytes() << std::endl;
  return v1.as<size_type>().get_bytes() == v2.as<size_type>().get_bytes();
}

template<>
inline bool not_equals<size_type>(const immutable_raw_data& v1,
                                  const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 16");
}

template<>
byte_string key_transform<size_type>(const immutable_raw_data& v,
                                     double bucket_size) {
  //THROW(unsupported_exception, "operation not yet supported 17");
  return byte_string(
      static_cast<uint64_t>(v.as<size_type>().get_bytes() / bucket_size));
}

static binary_ops_t get_binarops() {
  return {add<size_type>, subtract<size_type>, multiply<size_type>,
    divide<size_type>, modulo<size_type>, bw_and<size_type>,
    bw_or<size_type>, bw_xor<size_type>, bw_lshift<size_type>,
    bw_rshift<size_type>};
}

static unary_ops_t get_unarops() {
  return {assign<size_type>, negative<size_type>,
    positive<size_type>, bw_not<size_type>};
}

static rel_ops_t get_reops() {
  return {less_than<size_type>, less_than_equals<size_type>,
    greater_than<size_type>, greater_than_equals<size_type>,
    equals<size_type>, not_equals<size_type>};
}

static key_op_t get_keops() {
  return key_transform<size_type> ;
}

}

#endif /* CONFLUO_TEST_SIZE_TYPE_H_ */

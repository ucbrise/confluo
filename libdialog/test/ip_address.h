#ifndef DIALOG_IP_ADDRESS_H_
#define DIALOG_IP_ADDRESS_H_

#include <cstdint>
#include <regex>
#include <stdio.h>

#include "../dialog/types/immutable_value.h"
#include "types/byte_string.h"
#include "types/data_types.h"
#include "exceptions.h"

namespace dialog {

class ip_address {
 public:
  ip_address() {
    address = 0;
  }

  ip_address(uint32_t value) {
    address = value;
  }

  static std::string name() {
    return "ip_address";
  }

  std::string to_string() const {
    uint8_t bytes[4];
    bytes[0] = address & 0xFF;
    bytes[1] = (address >> 8) & 0xFF;
    bytes[2] = (address >> 16) & 0xFF;
    bytes[3] = (address >> 24) & 0xFF;
    char buf[100];
    sprintf(buf, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
    return std::string(buf);
  }

  static ip_address from_string(const std::string& str) {
    std::regex re(
        "^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])$");
    std::smatch str_matches;
    if (std::regex_search(str, str_matches, re)) {
      const int num_bytes = 4;
      int bytes[num_bytes];

      for (unsigned int i = 0; i < num_bytes; i++) {
        bytes[i] = utils::string_utils::lexical_cast<int>(
            str_matches.str(i + 1));
      }

      uint32_t addr = 256 * 256 * 256 * bytes[0] + 256 * 256 * bytes[1]
          + 256 * bytes[2] + bytes[3];
      return ip_address(addr);
    }
    THROW(unsupported_exception, "not valid ip");
  }

  static void parse_ip(const std::string& str, mutable_raw_data& out) {
    out.set(ip_address::from_string(str));
  }

  std::string repr() {
    return std::to_string(address);
  }

  static mutable_raw_data parse_ip_value(uint32_t val) {
    return mutable_raw_data(sizeof(ip_address)).set(val);
  }

  static std::string ip_to_string(const immutable_raw_data& data) {
    return data.as<ip_address>().to_string();
  }

  uint32_t get_address() const {
    return address;
  }
  void set_address(uint32_t addr) {
    address = addr;
  }

 private:
  uint32_t address;
} __attribute__((packed));

template<>
void serialize<ip_address>(std::ostream& out, const immutable_raw_data& value) {
  ip_address val = value.as<ip_address>();
  uint32_t val_address = val.get_address();
  out.write(reinterpret_cast<const char*>(&(val_address)), value.size);

}

template<>
void deserialize<ip_address>(std::istream& in, mutable_raw_data& out) {
  in.read(reinterpret_cast<char*>(out.ptr), sizeof(uint32_t));
}

template<>
inline void add<ip_address>(void* res, const immutable_raw_data& v1,
                            const immutable_raw_data& v2) {
  (*(reinterpret_cast<ip_address*>(res))).set_address(
      v1.as<ip_address>().get_address() + v2.as<ip_address>().get_address());
}

template<>
inline void subtract<ip_address>(void* res, const immutable_raw_data& v1,
                                 const immutable_raw_data& v2) {
  (*(reinterpret_cast<ip_address*>(res))).set_address(
      v1.as<ip_address>().get_address() - v2.as<ip_address>().get_address());
}

template<>
inline void multiply<ip_address>(void* res, const immutable_raw_data& v1,
                                 const immutable_raw_data& v2) {
  (*(reinterpret_cast<ip_address*>(res))).set_address(
      v1.as<ip_address>().get_address() * v2.as<ip_address>().get_address());

}

template<>
inline void divide<ip_address>(void* res, const immutable_raw_data& v1,
                               const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 2");
}

template<>
inline void modulo<ip_address>(void* res, const immutable_raw_data& v1,
                               const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 3");
}

template<>
inline void bw_and<ip_address>(void* res, const immutable_raw_data& v1,
                               const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 4");
}

template<>
inline void bw_or<ip_address>(void* res, const immutable_raw_data& v1,
                              const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 5");
}

template<>
inline void bw_xor<ip_address>(void* res, const immutable_raw_data& v1,
                               const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 6");
}

template<>
inline void bw_lshift<ip_address>(void* res, const immutable_raw_data& v1,
                                  const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 7");
}

template<>
inline void bw_rshift<ip_address>(void* res, const immutable_raw_data& v1,
                                  const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 8");
}

template<>
inline void assign<ip_address>(void* res, const immutable_raw_data& v) {
  //THROW(unsupported_exception, "operation not yet supported 9");
  (*(reinterpret_cast<ip_address*>(res))).set_address(
      v.as<ip_address>().get_address());
}

template<>
inline void negative<ip_address>(void* res, const immutable_raw_data& v) {
  (*(reinterpret_cast<ip_address*>(res))).set_address(
      -v.as<ip_address>().get_address());
}

template<>
inline void positive<ip_address>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "operation not yet supported 10");
}

template<>
inline void bw_not<ip_address>(void* res, const immutable_raw_data& v) {
  THROW(unsupported_exception, "operation not yet supported 11");
}

template<>
inline bool less_than<ip_address>(const immutable_raw_data& v1,
                                  const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 12");
}

template<>
inline bool less_than_equals<ip_address>(const immutable_raw_data& v1,
                                         const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 13");
}

template<>
inline bool greater_than<ip_address>(const immutable_raw_data& v1,
                                     const immutable_raw_data& v2) {
  return v1.as<ip_address>().get_address() > v2.as<ip_address>().get_address();
  //THROW(unsupported_exception, "operation not supported 69");
}

template<>
inline bool greater_than_equals<ip_address>(const immutable_raw_data& v1,
                                            const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 14");
}

template<>
inline bool equals<ip_address>(const immutable_raw_data& v1,
                               const immutable_raw_data& v2) {
  return v1.as<ip_address>().get_address() == v2.as<ip_address>().get_address();
  //THROW(unsupported_exception, "operation not yet supported 15");
}

template<>
inline bool not_equals<ip_address>(const immutable_raw_data& v1,
                                   const immutable_raw_data& v2) {
  THROW(unsupported_exception, "operation not yet supported 16");
}

template<>
byte_string key_transform<ip_address>(const immutable_raw_data& v,
                                      double bucket_size) {
  //THROW(unsupported_exception, "operation not yet supported 17");
  return byte_string(
      static_cast<uint32_t>(v.as<ip_address>().get_address() / bucket_size));
}

static binary_ops_t get_binaryops() {
  return {add<ip_address>, subtract<ip_address>, multiply<ip_address>,
    divide<ip_address>, modulo<ip_address>, bw_and<ip_address>,
    bw_or<ip_address>, bw_xor<ip_address>, bw_lshift<ip_address>,
    bw_rshift<ip_address>};
}

static unary_ops_t get_unaryops() {
  return {assign<ip_address>, negative<ip_address>,
    positive<ip_address>, bw_not<ip_address>};
}

static rel_ops_t get_relops() {
  return {less_than<ip_address>, less_than_equals<ip_address>,
    greater_than<ip_address>, greater_than_equals<ip_address>,
    equals<ip_address>, not_equals<ip_address>};
}

static key_op_t get_keyops() {
  return key_transform<ip_address> ;
}

}

#endif 

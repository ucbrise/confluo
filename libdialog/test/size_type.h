#ifndef DIALOG_SIZE_TYPE_H_
#define DIALOG_SIZE_TYPE_H_

#include <cstdint>
#include <regex>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "data_types.h"
#include "byte_string.h"
#include "immutable_value.h"
#include "exceptions.h"

namespace dialog {

class size_type {
  public:
   size_type() {
       bytes = 0;
   }

   size_type(uint64_t value) {
       bytes = value;
   }

   static std::string to_string() {
       return "size type";
   }

   static size_type from_string(const std::string& str) {
        std::regex re("(\\d+(?:\\.\\d+)?)\\s*([kmgtp]?b)");
        std::smatch str_matches;
        if (std::regex_search(str, str_matches, re)) {
            const int num_order = 6;
            std::string sizes[num_order] = {"b", "kb", "mb", "gb", "tb",
                "pb"};

            uint64_t value = utils::string_utils::lexical_cast<uint64_t>(
                    str_matches.str(1));

            std::string type = str_matches.str(2);
            std::transform(type.begin(), type.end(), type.begin(), 
                    ::tolower);
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

   static data parse_bytes(const std::string& str) {
        size_type *size = new size_type(size_type::from_string(str).
                get_bytes());
        const void* val = (void *) size;
        return data(val, sizeof(uint64_t));
   }

   static data parse_bytes(uint64_t _bytes) {
       size_type *size = new size_type(_bytes);
       const void* val = (void*) size;
       return data(val, sizeof(uint64_t));
   }

   std::string repr() {
       return std::to_string(bytes);
   }

   uint64_t get_bytes() const { return bytes; }
   void set_bytes(uint64_t _bytes) { bytes = _bytes; }

 private:
  uint64_t bytes;
};

/*mutable_value parse_ip_address(const std::string& str) {
    uint32_t val = size_type::from_string(str);
    int32_t mut_val = (int32_t) val;
    return mutable_value(mut_val);
}*/

template<>
void serialize<size_type>(std::ostream& out, data& value) {
    size_type val = value.as<size_type>();
    out.write(reinterpret_cast<const char*>(&(value.ptr)), value.size);
}

template<>
data deserialize<size_type>(std::istream& in) {
    uint64_t val;
    in.read(reinterpret_cast<char*>(&val), sizeof(uint64_t));
    return size_type::parse_bytes(val);
}

template<>
inline void add<size_type>(void* res, const data& v1, const data& v2) {
    (*(reinterpret_cast<size_type*>(res))).set_bytes(
        v1.as<size_type>().get_bytes() + 
        v2.as<size_type>().get_bytes());
}

template<>
inline void subtract<size_type>(void* res, const data& v1, 
        const data& v2) {
    (*(reinterpret_cast<size_type*>(res))).set_bytes(
        v1.as<size_type>().get_bytes()  -
        v2.as<size_type>().get_bytes());
}

template<>
inline void multiply<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 1");
}

template<>
inline void divide<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 2");
}

template<>
inline void modulo<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 3");
}
   
template<>
inline void bw_and<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 4");
}

template<>
inline void bw_or<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 5");
}

template<>
inline void bw_xor<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 6");
}

template<>
inline void bw_lshift<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 7");
}

template<>
inline void bw_rshift<size_type>(void* res, const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 8");
}

template<>
inline void assign<size_type>(void* res, const data& v) {
    (*(reinterpret_cast<size_type*>(res))).set_bytes(v.as<size_type>().get_bytes());
}

template<>
inline void negative<size_type>(void* res, const data& v) {
    (*(reinterpret_cast<size_type*>(res))).set_bytes(
            -v.as<size_type>().get_bytes());
}

template<>
inline void positive<size_type>(void* res, const data& v) {
    THROW(unsupported_exception, "operation not yet supported 10");
}
    
template<>
inline void bw_not<size_type>(void* res, const data& v) {
    THROW(unsupported_exception, "operation not yet supported 11");
}

template<>
inline bool less_than<size_type>(const data& v1, const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 12");
}

template<>
inline bool less_than_equals<size_type>(const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 13");
}

template<>
inline bool greater_than<size_type>(const data& v1, const data& v2) {
    return v1.as<size_type>().get_bytes() > 
        v2.as<size_type>().get_bytes();
       //THROW(unsupported_exception, "operation not supported 69");
}

template<>
inline bool greater_than_equals<size_type>(const data& v1, 
        const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 14");
}

template<>
inline bool equals<size_type>(const data& v1, const data& v2) {
       //std::cout << "v1: " << v1.as<size_type>().get_bytes() << std::endl;
       //std::cout << "v2: " << v2.as<size_type>().get_bytes() << std::endl;
    return v1.as<size_type>().get_bytes() ==
        v2.as<size_type>().get_bytes();
}

template<>
inline bool not_equals<size_type>(const data& v1, const data& v2) {
    THROW(unsupported_exception, "operation not yet supported 16");
}

template<>
byte_string key_transform<size_type>(const data& v, 
        double bucket_size) {
       //THROW(unsupported_exception, "operation not yet supported 17");
    return byte_string(static_cast<uint64_t>(v.as<size_type>(
                       ).get_bytes() / bucket_size));
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

static key_op get_keops() {
    return key_transform<size_type>;
}


}

#endif 

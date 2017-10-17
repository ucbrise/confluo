#ifndef DIALOG_IP_ADDRESS_H_
#define DIALOG_IP_ADDRESS_H_

#include <cstdint>
#include <regex>
#include <stdio.h>

#include "data_types.h"
#include "byte_string.h"
#include "immutable_value.h"
#include "exceptions.h"

namespace dialog {

class ip_address {
  public:
   ip_address(std::string address_string) {
   }

   std::string to_string() const {
   }

   bool is_valid(std::string address_string) {
       std::regex re("^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])$");
       std::cmatch match;
       return std::regex_match(address_string, match, re);
   }

   void parse(std::string address_string) {
       if (!is_valid(address_string)) {
           return;
       }

       unsigned byte bytes[4];
       sscanf(address_string, "%d.%d.%d.%d", bytes, bytes + 1, bytes + 2,
               bytes + 3);
       address = (256 * 256 * 256 * bytes[0]) + (256 * 256 * bytes[1]) +
           (256 * bytes[2]) + bytes[3]l
   }

   inline void add(void* res, const data& v1, const data& v2) {
      (*(reinterpret_cast<T*>(res))).address = v1.as<T>().address +
          v2.as<T>().address;
   }

   inline void subtract(void* res, const data& v1, const data& v2) {
      (*(reinterpret_cast<T*>(res))).address = v1.as<T>().address -
          v2.as<T>().address;
   }


   inline void multiply(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }

   inline void divide(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }

   inline void modulo(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }
   
   inline void bw_and(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }

   inline void bw_or(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }
   
   inline void bw_xor(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }
   
   inline void bw_lshift(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }
   
   inline void bw_rshift(void* res, const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported");
   }

   static binary_ops_t get_binaryops() {
       return {add, subtract, multiply, divide, modulo, bw_and, bw_or,
           bw_xor, bw_lshift, bw_rshift};
   }


 private:
  uint32_t address;
};

}

#endif 

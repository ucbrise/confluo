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
       if (is_valid(address_string)) {
           parse(address_string);
       }
   }

   ip_address() {
       address = 0;
   }

   ip_address(uint32_t value) {
       address = value;
   }

   static std::string to_string() {
       return "ip_address";
   }

   bool is_valid(std::string address_string) {
       /*std::regex re("^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])$");
       std::cmatch match;
       return std::regex_match(address_string, match, re);*/
       return true;
   }

   void parse(std::string address_string) {
       /*if (!is_valid(address_string)) {
           return;
       }

       unsigned char bytes[4] = {0, 0, 0, 0};
       sscanf(address_string, "%d.%d.%d.%d", bytes, bytes + 1, bytes + 2,
               bytes + 3);
       address = (256 * 256 * 256 * bytes[0]) + (256 * 256 * bytes[1]) +
           (256 * bytes[2]) + bytes[3];*/
       address = 9000;
   }

   uint32_t get_address() { return address; }
   void set_address(uint32_t addr) { address = addr; }

 private:
  uint32_t address;
};


template<>
inline void add<ip_address>(void* res, const data& v1, const data& v2) {
    (*(reinterpret_cast<ip_address*>(res))).set_address(
        v1.as<ip_address>().get_address() + 
        v2.as<ip_address>().get_address());
}

   template<>
   inline void subtract<ip_address>(void* res, const data& v1, 
           const data& v2) {
      (*(reinterpret_cast<ip_address*>(res))).set_address(
          v1.as<ip_address>().get_address()  -
          v2.as<ip_address>().get_address());
   }

   template<>
   inline void multiply<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 1");
   }

   template<>
   inline void divide<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 2");
   }

   template<>
   inline void modulo<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 3");
   }
   
   template<>
   inline void bw_and<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 4");
   }

   template<>
   inline void bw_or<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 5");
   }

   template<>
   inline void bw_xor<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 6");
   }

   template<>
   inline void bw_lshift<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 7");
   }

   template<>
   inline void bw_rshift<ip_address>(void* res, const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 8");
   }

   template<>
   inline void assign<ip_address>(void* res, const data& v) {
       //THROW(unsupported_exception, "operation not yet supported 9");
       (*(reinterpret_cast<ip_address*>(res))).set_address(v.as<ip_address>().get_address());
   }

   template<>
   inline void negative<ip_address>(void* res, const data& v) {
       (*(reinterpret_cast<ip_address*>(res))).set_address(
               -v.as<ip_address>().get_address());
   }

   template<>
   inline void positive<ip_address>(void* res, const data& v) {
       THROW(unsupported_exception, "operation not yet supported 10");
   }
    
   template<>
   inline void bw_not<ip_address>(void* res, const data& v) {
       THROW(unsupported_exception, "operation not yet supported 11");
   }

   template<>
   inline bool less_than<ip_address>(const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 12");
   }

   template<>
   inline bool less_than_equals<ip_address>(const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 13");
   }

   template<>
   inline bool greater_than<ip_address>(const data& v1, const data& v2) {
       return v1.as<ip_address>().get_address() > 
           v2.as<ip_address>().get_address();
   }

   template<>
   inline bool greater_than_equals<ip_address>(const data& v1, 
           const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 14");
   }

   template<>
   inline bool equals<ip_address>(const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 15");
   }

   template<>
   inline bool not_equals<ip_address>(const data& v1, const data& v2) {
       THROW(unsupported_exception, "operation not yet supported 16");
   }

   template<>
   byte_string key_transform<ip_address>(const data& v, 
           double bucket_size) {
       THROW(unsupported_exception, "operation not yet supported 17");
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

   static key_op get_keyops() {
       return key_transform<ip_address>;
   }


}

#endif 

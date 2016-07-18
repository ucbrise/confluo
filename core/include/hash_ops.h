#ifndef HASH_OPS_H_
#define HASH_OPS_H_

#include "flags.h"

class Hash {
 public:
  static const uint32_t K1 = 256;
  static const uint32_t K2 = 65536;
  static const uint32_t K3 = 16777216;

  static uint32_t simple_hash(const char* buf) {
    uint8_t* cast_buf = (uint8_t*) buf;
#ifdef HASH4
    return cast_buf[0] * K3 + cast_buf[1] * K2 + cast_buf[2] * K1 + cast_buf[3];
#else
#ifdef HASH3
    return cast_buf[0] * K2 + cast_buf[1] * K1 + cast_buf[2];
#else
    return cast_buf[0] * K1 + cast_buf[1];
#endif
#endif
  }
};

#endif /* HASH_OPS_H_ */

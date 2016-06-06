#ifndef HASH_OPS_H_
#define HASH_OPS_H_

class Hash {
 public:
  static const uint32_t K1 = 256;
  static const uint32_t K2 = 65536;
  static const uint32_t K3 = 16777216;

  static uint32_t simple_hash2(const char* buf) {
    return buf[0] * K1 + buf[1];
  }

  static uint32_t simple_hash3(const char* buf) {
    return buf[0] * K2 + buf[1] * K1 + buf[2];
  }

  static uint32_t simple_hash4(const char* buf) {
    return buf[0] * K3 + buf[1] * K2 + buf[2] * K1 + buf[3];
  }
};



#endif /* HASH_OPS_H_ */

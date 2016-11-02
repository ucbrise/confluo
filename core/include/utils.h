#ifndef SLOG_UTILS_H_
#define SLOG_UTILS_H_

#include <cstdint>

#define BSR

namespace slog {

class bit_utils {
 public:
  static inline uint32_t highest_bit(uint32_t x) {
    uint32_t y = 0;
#ifdef BSR
    asm ( "\tbsr %1, %0\n"
        : "=r"(y)
        : "r" (x)
    );
#else
    while (x >>= 1)
    ++y;
#endif
    return y;
  }
};

class cast_utils {
 public:
  static inline uint64_t as_uint64_1(const unsigned char* buf) {
    return (uint64_t) buf[0];
  }

  static inline uint64_t as_uint64_2(const unsigned char* buf) {
    return (uint64_t) (((uint64_t) buf[0] << 8) | ((uint64_t) buf[1]));
  }

  static inline uint64_t as_uint64_3(const unsigned char* buf) {
    return (((uint64_t) buf[0] << 16) | ((uint64_t) buf[1] << 8)
        | ((uint64_t) buf[2]));
  }

  static inline uint64_t as_uint64_4(const unsigned char* buf) {
    return (((uint64_t) buf[0] << 24) | ((uint64_t) buf[1] << 16)
        | ((uint64_t) buf[2] << 8) | ((uint64_t) buf[3]));
  }

  static inline uint64_t as_uint64_5(const unsigned char* buf) {
    return (((uint64_t) buf[0] << 32) | ((uint64_t) buf[1] << 24)
        | ((uint64_t) buf[2] << 16) | ((uint64_t) buf[3] << 8)
        | ((uint64_t) buf[4]));
  }

  static inline uint64_t as_uint64_6(const unsigned char* buf) {
    return (((uint64_t) buf[0] << 40) | ((uint64_t) buf[1] << 32)
        | ((uint64_t) buf[2] << 24) | ((uint64_t) buf[3] << 16)
        | ((uint64_t) buf[4] << 8) | ((uint64_t) buf[5]));
  }

  static inline uint64_t as_uint64_7(const unsigned char* buf) {
    return (((uint64_t) buf[0] << 48) | ((uint64_t) buf[1] << 40)
        | ((uint64_t) buf[2] << 32) | ((uint64_t) buf[3] << 24)
        | ((uint64_t) buf[4] << 16) | ((uint64_t) buf[5] << 8)
        | ((uint64_t) buf[6]));
  }

  static inline uint64_t as_uint64_8(const unsigned char* buf) {
    return (((uint64_t) buf[0] << 54) | ((uint64_t) buf[1] << 48)
        | ((uint64_t) buf[2] << 40) | ((uint64_t) buf[3] << 32)
        | ((uint64_t) buf[4] << 24) | ((uint64_t) buf[5] << 16)
        | ((uint64_t) buf[6] << 8) | ((uint64_t) buf[7]));
  }

  static inline uint64_t as_uint64(const unsigned char* buf, size_t len) {
    switch (len) {
      case 1:
        return as_uint64_1(buf);
      case 2:
        return as_uint64_2(buf);
      case 3:
        return as_uint64_3(buf);
      case 4:
        return as_uint64_4(buf);
      case 5:
        return as_uint64_5(buf);
      case 6:
        return as_uint64_6(buf);
      case 7:
        return as_uint64_7(buf);
      case 8:
        return as_uint64_8(buf);
    }
    return 0;
  }
};

}

#endif /* SLOG_UTILS_H_ */

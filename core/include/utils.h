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

}

#endif /* SLOG_UTILS_H_ */

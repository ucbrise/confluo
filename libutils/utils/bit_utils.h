#ifndef UTILS_BIT_UTILS_H_
#define UTILS_BIT_UTILS_H_

namespace utils {

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

#endif /* UTILS_BIT_UTILS_H_ */

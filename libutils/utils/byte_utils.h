#ifndef UTILS_BYTE_UTILS_H_
#define UTILS_BYTE_UTILS_H_

#define CONFLUO_UNKNOWN_ENDIAN 0
#define CONFLUO_BIG_ENDIAN     1
#define CONFLUO_LITTLE_ENDIAN 2

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define CONFLUO_ENDIANNESS CONFLUO_BIG_ENDIAN
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define CONFLUO_ENDIANNESS CONFLUO_LITTLE_ENDIAN
#else
#define CONFLUO_ENDIANNESS CONFLUO_UNKNOWN_ENDIAN
#endif

namespace utils {

class byte_utils {
 public:
  static inline bool is_big_endian() {
    int num = 1;
    return *(char *) &num != 1;
  }

  static inline uint8_t byte_swap(uint8_t __val) {
    return __val;
  }

  static inline int8_t byte_swap(int8_t __val) {
    return __val;
  }

  static inline uint16_t byte_swap(uint16_t __val) {
    return (__val << 8) | (__val >> 8);
  }

  static inline int16_t byte_swap(int16_t __val) {
    return static_cast<int16_t>((__val << 8) | ((__val >> 8) & 0xFF));
  }

  static inline uint32_t byte_swap(uint32_t __val) {
    __val = ((__val << 8) & 0xFF00FF00) | ((__val >> 8) & 0xFF00FF);
    return (__val << 16) | (__val >> 16);
  }

  static inline int32_t byte_swap(int32_t __val) {
    __val = ((__val << 8) & 0xFF00FF00) | ((__val >> 8) & 0xFF00FF);
    return (__val << 16) | ((__val >> 16) & 0xFFFF);
  }

  static inline uint64_t byte_swap(uint64_t __val) {
    __val = ((__val << 8) & 0xFF00FF00FF00FF00ULL)
        | ((__val >> 8) & 0x00FF00FF00FF00FFULL);
    __val = ((__val << 16) & 0xFFFF0000FFFF0000ULL)
        | ((__val >> 16) & 0x0000FFFF0000FFFFULL);
    return (__val << 32) | (__val >> 32);
  }

  static inline int64_t byte_swap(int64_t __val) {
    __val = ((__val << 8) & 0xFF00FF00FF00FF00ULL)
        | ((__val >> 8) & 0x00FF00FF00FF00FFULL);
    __val = ((__val << 16) & 0xFFFF0000FFFF0000ULL)
        | ((__val >> 16) & 0x0000FFFF0000FFFFULL);
    return (__val << 32) | ((__val >> 32) & 0xFFFFFFFFULL);
  }

  template<typename T>
  static T reverse_as(uint8_t* data, size_t size) {
    uint8_t *buf = new uint8_t[size];
    for (size_t i = 0; i < size; i++) {
      buf[size - i - 1] = data[i];
    }
    T val = *reinterpret_cast<T*>(buf);
    delete[] buf;
    return val;
  }

};

}

#endif /* UTILS_BYTE_UTILS_H_ */

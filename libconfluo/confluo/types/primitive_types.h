#ifndef CONFLUO_TYPES_PRIMITIVE_TYPES_H_
#define CONFLUO_TYPES_PRIMITIVE_TYPES_H_

#include <limits>
#include <cstdint>

namespace confluo {

/**
 * Identification of primitive types
 */
enum primitive_type
    : uint16_t {
  /** Identifier for the none type */
      D_NONE = 0,
  /** Identifier for the boolean type */
      D_BOOL = 1,
  /** Identifier for the character type */
      D_CHAR = 2,
  /** Identifier for the unsigned character type */
      D_UCHAR = 3,
  /** Identifier for the short type */
      D_SHORT = 4,
  /** Identifier for the unsigned short type */
      D_USHORT = 5,
  /** Identifier for the signed integer type */
      D_INT = 6,
  /** Identifier for the unsigned integer type */
      D_UINT = 7,
  /** Identifier for the long type */
      D_LONG = 8,
  /** Identifier for the unsigned long type */
      D_ULONG = 9,
  /** Identifier for the float type */
      D_FLOAT = 10,
  /** Identifier for the double type */
      D_DOUBLE = 11,
  /** Identifier for the string type */
      D_STRING = 12
};

class limits {
 public:
  /** The minimum boolean value */
  static bool bool_min;
  /** The zero boolean value */
  static bool bool_zero;
  /** The boolean value representing one */
  static bool bool_one;
  /** The max boolean value */
  static bool bool_max;

  /** The minimum character value */
  static int8_t char_min;
  /** The zero character value */
  static int8_t char_zero;
  /** The character value representing one */
  static int8_t char_one;
  /** The max character value */
  static int8_t char_max;

  /** The minimum unsigned character */
  static uint8_t uchar_min;
  /** The unsigned character value representing zero */
  static uint8_t uchar_zero;
  /** The unsigned character value representing one */
  static uint8_t uchar_one;
  /** The maximum unsigned character value */
  static uint8_t uchar_max;

  /** The minimum short value */
  static int16_t short_min;
  /** The zero short value */
  static int16_t short_zero;
  /** The short value representing one */
  static int16_t short_one;
  /** The max short value */
  static int16_t short_max;

  /** The minimum unsigned short value */
  static uint16_t ushort_min;
  /** The zero unsigned short value */
  static uint16_t ushort_zero;
  /** The unsigned short value representing one */
  static uint16_t ushort_one;
  /** The maximum unsigned short value */
  static uint16_t ushort_max;

  /** The minimum integer value */
  static int32_t int_min;
  /** The zero integer value */
  static int32_t int_zero;
  /** The integer value representing one */
  static int32_t int_one;
  /** The max integer value */
  static int32_t int_max;

  /** The minimum unsigned integer value */
  static uint32_t uint_min;
  /** The zero unsigned integer value */
  static uint32_t uint_zero;
  /** The unsigned integer value representing one */
  static uint32_t uint_one;
  /** The maximum unsigned integer value */
  static uint32_t uint_max;

  /** The minimum long value */
  static int64_t long_min;
  /** The zero long value */
  static int64_t long_zero;
  /** The long value representing one */
  static int64_t long_one;
  /** The max long value */
  static int64_t long_max;

  /** The minimum unsigned long value */
  static uint64_t ulong_min;
  /** The zero unsigned long value */
  static uint64_t ulong_zero;
  /** The unsigned long value representing one */
  static uint64_t ulong_one;
  /** The max unsigned long value */
  static uint64_t ulong_max;

  /** The minimum float value */
  static float float_min;
  /** The zero float value */
  static float float_zero;
  /** The float value representing one */
  static float float_one;
  /** The max float value */
  static float float_max;

  /** The minimum double value */
  static double double_min;
  /** The zero double value */
  static double double_zero;
  /** The double value representing one */
  static double double_one;
  /** The max double value */
  static double double_max;
};

}

#endif /* CONFLUO_TYPES_PRIMITIVE_TYPES_H_ */

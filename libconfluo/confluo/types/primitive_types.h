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

namespace limits {

/** The minimum boolean value */
extern bool bool_min;
/** The zero boolean value */
extern bool bool_zero;
/** The boolean value representing one */
extern bool bool_one;
/** The max boolean value */
extern bool bool_max;

/** The minimum character value */
extern int8_t char_min;
/** The zero character value */
extern int8_t char_zero;
/** The character value representing one */
extern int8_t char_one;
/** The max character value */
extern int8_t char_max;

/** The minimum unsigned character */
extern uint8_t uchar_min;
/** The unsigned character value representing zero */
extern uint8_t uchar_zero;
/** The unsigned character value representing one */
extern uint8_t uchar_one;
/** The maximum unsigned character value */
extern uint8_t uchar_max;

/** The minimum short value */
extern int16_t short_min;
/** The zero short value */
extern int16_t short_zero;
/** The short value representing one */
extern int16_t short_one;
/** The max short value */
extern int16_t short_max;

/** The minimum unsigned short value */
extern uint16_t ushort_min;
/** The zero unsigned short value */
extern uint16_t ushort_zero;
/** The unsigned short value representing one */
extern uint16_t ushort_one;
/** The maximum unsigned short value */
extern uint16_t ushort_max;

/** The minimum integer value */
extern int32_t int_min;
/** The zero integer value */
extern int32_t int_zero;
/** The integer value representing one */
extern int32_t int_one;
/** The max integer value */
extern int32_t int_max;

/** The minimum unsigned integer value */
extern uint32_t uint_min;
/** The zero unsigned integer value */
extern uint32_t uint_zero;
/** The unsigned integer value representing one */
extern uint32_t uint_one;
/** The maximum unsigned integer value */
extern uint32_t uint_max;

/** The minimum long value */
extern int64_t long_min;
/** The zero long value */
extern int64_t long_zero;
/** The long value representing one */
extern int64_t long_one;
/** The max long value */
extern int64_t long_max;

/** The minimum unsigned long value */
extern uint64_t ulong_min;
/** The zero unsigned long value */
extern uint64_t ulong_zero;
/** The unsigned long value representing one */
extern uint64_t ulong_one;
/** The max unsigned long value */
extern uint64_t ulong_max;

/** The minimum float value */
extern float float_min;
/** The zero float value */
extern float float_zero;
/** The float value representing one */
extern float float_one;
/** The max float value */
extern float float_max;

/** The minimum double value */
extern double double_min;
/** The zero double value */
extern double double_zero;
/** The double value representing one */
extern double double_one;
/** The max double value */
extern double double_max;
}

}

#endif /* CONFLUO_TYPES_PRIMITIVE_TYPES_H_ */

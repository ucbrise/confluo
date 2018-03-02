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
static bool bool_min = std::numeric_limits<bool>::lowest();
/** The zero boolean value */
static bool bool_zero = false;
/** The boolean value representing one */
static bool bool_one = true;
/** The max boolean value */
static bool bool_max = std::numeric_limits<bool>::max();

/** The minimum character value */
static int8_t char_min = std::numeric_limits<int8_t>::lowest();
/** The zero character value */
static int8_t char_zero = static_cast<int8_t>(0);
/** The character value representing one */
static int8_t char_one = static_cast<int8_t>(1);
/** The max character value */
static int8_t char_max = std::numeric_limits<int8_t>::max();

/** The minimum unsigned character */
static uint8_t uchar_min = std::numeric_limits<uint8_t>::lowest();
/** The unsigned character value representing zero */
static uint8_t uchar_zero = static_cast<uint8_t>(0);
/** The unsigned character value representing one */
static uint8_t uchar_one = static_cast<uint8_t>(1);
/** The maximum unsigned character value */
static uint8_t uchar_max = std::numeric_limits<uint8_t>::max();

/** The minimum short value */
static int16_t short_min = std::numeric_limits<int16_t>::lowest();
/** The zero short value */
static int16_t short_zero = static_cast<int16_t>(0);
/** The short value representing one */
static int16_t short_one = static_cast<int16_t>(1);
/** The max short value */
static int16_t short_max = std::numeric_limits<int16_t>::max();

/** The minimum unsigned short value */
static uint16_t ushort_min = std::numeric_limits<uint16_t>::lowest();
/** The zero unsigned short value */
static uint16_t ushort_zero = static_cast<uint16_t>(0);
/** The unsigned short value representing one */
static uint16_t ushort_one = static_cast<uint16_t>(1);
/** The maximum unsigned short value */
static uint16_t ushort_max = std::numeric_limits<uint16_t>::max();

/** The minimum integer value */
static int32_t int_min = std::numeric_limits<int32_t>::lowest();
/** The zero integer value */
static int32_t int_zero = static_cast<int32_t>(0);
/** The integer value representing one */
static int32_t int_one = static_cast<int32_t>(1);
/** The max integer value */
static int32_t int_max = std::numeric_limits<int32_t>::max();

/** The minimum unsigned integer value */
static uint32_t uint_min = std::numeric_limits<uint32_t>::lowest();
/** The zero unsigned integer value */
static uint32_t uint_zero = static_cast<uint32_t>(0);
/** The unsigned integer value representing one */
static uint32_t uint_one = static_cast<uint32_t>(1);
/** The maximum unsigned integer value */
static uint32_t uint_max = std::numeric_limits<uint32_t>::max();

/** The minimum long value */
static int64_t long_min = std::numeric_limits<int64_t>::lowest();
/** The zero long value */
static int64_t long_zero = static_cast<int64_t>(0);
/** The long value representing one */
static int64_t long_one = static_cast<int64_t>(1);
/** The max long value */
static int64_t long_max = std::numeric_limits<int64_t>::max();

/** The minimum unsigned long value */
static uint64_t ulong_min = std::numeric_limits<uint64_t>::lowest();
/** The zero unsigned long value */
static uint64_t ulong_zero = static_cast<uint64_t>(0);
/** The unsigned long value representing one */
static uint64_t ulong_one = static_cast<uint64_t>(1);
/** The max unsigned long value */
static uint64_t ulong_max = std::numeric_limits<uint64_t>::max();

/** The minimum float value */
static float float_min = std::numeric_limits<float>::lowest();
/** The zero float value */
static float float_zero = static_cast<float>(0.0);
/** The float value representing one */
static float float_one = static_cast<float>(1.0);
/** The max float value */
static float float_max = std::numeric_limits<float>::max();

/** The minimum double value */
static double double_min = std::numeric_limits<double>::lowest();
/** The zero double value */
static double double_zero = static_cast<double>(0.0);
/** The double value representing one */
static double double_one = static_cast<double>(1.0);
/** The max double value */
static double double_max = std::numeric_limits<double>::max();
}

}

#endif /* CONFLUO_TYPES_PRIMITIVE_TYPES_H_ */

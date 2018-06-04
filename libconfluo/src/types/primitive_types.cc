#include "types/primitive_types.h"

namespace confluo {

/** The minimum boolean value */
bool limits::bool_min = std::numeric_limits<bool>::lowest();
/** The zero boolean value */
bool limits::bool_zero = false;
/** The boolean value representing one */
bool limits::bool_one = true;
/** The max boolean value */
bool limits::bool_max = std::numeric_limits<bool>::max();

/** The minimum character value */
int8_t limits::char_min = std::numeric_limits<int8_t>::lowest();
/** The zero character value */
int8_t limits::char_zero = static_cast<int8_t>(0);
/** The character value representing one */
int8_t limits::char_one = static_cast<int8_t>(1);
/** The max character value */
int8_t limits::char_max = std::numeric_limits<int8_t>::max();

/** The minimum unsigned character */
uint8_t limits::uchar_min = std::numeric_limits<uint8_t>::lowest();
/** The unsigned character value representing zero */
uint8_t limits::uchar_zero = static_cast<uint8_t>(0);
/** The unsigned character value representing one */
uint8_t limits::uchar_one = static_cast<uint8_t>(1);
/** The maximum unsigned character value */
uint8_t limits::uchar_max = std::numeric_limits<uint8_t>::max();

/** The minimum short value */
int16_t limits::short_min = std::numeric_limits<int16_t>::lowest();
/** The zero short value */
int16_t limits::short_zero = static_cast<int16_t>(0);
/** The short value representing one */
int16_t limits::short_one = static_cast<int16_t>(1);
/** The max short value */
int16_t limits::short_max = std::numeric_limits<int16_t>::max();

/** The minimum unsigned short value */
uint16_t limits::ushort_min = std::numeric_limits<uint16_t>::lowest();
/** The zero unsigned short value */
uint16_t limits::ushort_zero = static_cast<uint16_t>(0);
/** The unsigned short value representing one */
uint16_t limits::ushort_one = static_cast<uint16_t>(1);
/** The maximum unsigned short value */
uint16_t limits::ushort_max = std::numeric_limits<uint16_t>::max();

/** The minimum integer value */
int32_t limits::int_min = std::numeric_limits<int32_t>::lowest();
/** The zero integer value */
int32_t limits::int_zero = static_cast<int32_t>(0);
/** The integer value representing one */
int32_t limits::int_one = static_cast<int32_t>(1);
/** The max integer value */
int32_t limits::int_max = std::numeric_limits<int32_t>::max();

/** The minimum unsigned integer value */
uint32_t limits::uint_min = std::numeric_limits<uint32_t>::lowest();
/** The zero unsigned integer value */
uint32_t limits::uint_zero = static_cast<uint32_t>(0);
/** The unsigned integer value representing one */
uint32_t limits::uint_one = static_cast<uint32_t>(1);
/** The maximum unsigned integer value */
uint32_t limits::uint_max = std::numeric_limits<uint32_t>::max();

/** The minimum long value */
int64_t limits::long_min = std::numeric_limits<int64_t>::lowest();
/** The zero long value */
int64_t limits::long_zero = static_cast<int64_t>(0);
/** The long value representing one */
int64_t limits::long_one = static_cast<int64_t>(1);
/** The max long value */
int64_t limits::long_max = std::numeric_limits<int64_t>::max();

/** The minimum unsigned long value */
uint64_t limits::ulong_min = std::numeric_limits<uint64_t>::lowest();
/** The zero unsigned long value */
uint64_t limits::ulong_zero = static_cast<uint64_t>(0);
/** The unsigned long value representing one */
uint64_t limits::ulong_one = static_cast<uint64_t>(1);
/** The max unsigned long value */
uint64_t limits::ulong_max = std::numeric_limits<uint64_t>::max();

/** The minimum float value */
float limits::float_min = std::numeric_limits<float>::lowest();
/** The zero float value */
float limits::float_zero = static_cast<float>(0.0);
/** The float value representing one */
float limits::float_one = static_cast<float>(1.0);
/** The max float value */
float limits::float_max = std::numeric_limits<float>::max();

/** The minimum double value */
double limits::double_min = std::numeric_limits<double>::lowest();
/** The zero double value */
double limits::double_zero = static_cast<double>(0.0);
/** The double value representing one */
double limits::double_one = static_cast<double>(1.0);
/** The max double value */
double limits::double_max = std::numeric_limits<double>::max();

}
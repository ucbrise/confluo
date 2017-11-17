#ifndef DIALOG_PRIMITIVE_TYPES_H_
#define DIALOG_PRIMITIVE_TYPES_H_

#include <limits>
#include <cstdint>

namespace confluo {

enum primitive_type_id
  : uint16_t {
    D_NONE = 0,
  D_BOOL = 1,
  D_CHAR = 2,
  D_SHORT = 3,
  D_INT = 4,
  D_LONG = 5,
  D_FLOAT = 6,
  D_DOUBLE = 7,
  D_STRING = 8
};

namespace limits {

static bool bool_min = std::numeric_limits<bool>::lowest();
static bool bool_zero = false;
static bool bool_one = true;
static bool bool_max = std::numeric_limits<bool>::max();

static int8_t char_min = std::numeric_limits<int8_t>::lowest();
static int8_t char_zero = static_cast<int8_t>(0);
static int8_t char_one = static_cast<int8_t>(1);
static int8_t char_max = std::numeric_limits<int8_t>::max();

static int16_t short_min = std::numeric_limits<int16_t>::lowest();
static int16_t short_zero = static_cast<int16_t>(0);
static int16_t short_one = static_cast<int16_t>(1);
static int16_t short_max = std::numeric_limits<int16_t>::max();

static int32_t int_min = std::numeric_limits<int32_t>::lowest();
static int32_t int_zero = static_cast<int32_t>(0);
static int32_t int_one = static_cast<int32_t>(1);
static int32_t int_max = std::numeric_limits<int32_t>::max();

static int64_t long_min = std::numeric_limits<int64_t>::lowest();
static int64_t long_zero = static_cast<int64_t>(0);
static int64_t long_one = static_cast<int64_t>(1);
static int64_t long_max = std::numeric_limits<int64_t>::max();

static uint64_t ulong_min = std::numeric_limits<uint64_t>::lowest();
static uint64_t ulong_zero = static_cast<uint64_t>(0);
static uint64_t ulong_one = static_cast<uint64_t>(1);
static uint64_t ulong_max = std::numeric_limits<uint64_t>::max();

static float float_min = std::numeric_limits<float>::lowest();
static float float_zero = static_cast<float>(0.0);
static float float_one = static_cast<float>(1.0);
static float float_max = std::numeric_limits<float>::max();

static double double_min = std::numeric_limits<double>::lowest();
static double double_zero = static_cast<double>(0.0);
static double double_one = static_cast<double>(1.0);
static double double_max = std::numeric_limits<double>::max();
}

}

#endif /* DIALOG_PRIMITIVE_TYPES_H_ */

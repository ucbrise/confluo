#ifndef CONFLUO_TEST_IMMUTABLE_VALUE_TEST_H_
#define CONFLUO_TEST_IMMUTABLE_VALUE_TEST_H_

#include "types/immutable_value.h"

#include "gtest/gtest.h"
#include "../test_utils.h"

using namespace ::confluo;

class ImmutableValueTest : public testing::Test {
 public:
  // Dummy function to stop compiler from complaining.
  static bool check(bool val) {
    return val;
  }
};

TEST_F(ImmutableValueTest, BoolValueTest) {
  bool v1 = true, v2 = false;

  immutable_value val1(BOOL_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const bool *>(val1.ptr()));

  immutable_value val2(BOOL_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const bool *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, CharValueTest) {
  int8_t v1 = 'c', v2 = '1';

  immutable_value val1(CHAR_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const int8_t *>(val1.ptr()));

  immutable_value val2(CHAR_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const int8_t *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, ShortValueTest) {
  int16_t v1 = 10, v2 = -100;

  immutable_value val1(SHORT_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const int16_t *>(val1.ptr()));

  immutable_value val2(SHORT_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const int16_t *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, IntValueTest) {
  int32_t v1 = 65536, v2 = -100;

  immutable_value val1(INT_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const int32_t *>(val1.ptr()));

  immutable_value val2(INT_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const int32_t *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, LongValueTest) {
  int64_t v1 = 2147483648, v2 = -100;
  immutable_value val1(LONG_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const int64_t *>(val1.ptr()));

  immutable_value val2(LONG_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const int64_t *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, FloatValueTest) {
  float v1 = 10.4, v2 = -100.3;

  immutable_value val1(FLOAT_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const float *>(val1.ptr()));

  immutable_value val2(FLOAT_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const float *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, DoubleValueTest) {
  double v1 = 10.4, v2 = -100.3;

  immutable_value val1(DOUBLE_TYPE, &v1);
  ASSERT_EQ(v1, *reinterpret_cast<const double *>(val1.ptr()));

  immutable_value val2(DOUBLE_TYPE, &v2);
  ASSERT_EQ(v2, *reinterpret_cast<const double *>(val2.ptr()));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, StringValueTest) {
  char v1[] = "abc", v2[] = "123";

  immutable_value val1(STRING_TYPE(64), v1);
  ASSERT_TRUE(strcmp(v1, reinterpret_cast<const char *>(val1.ptr())) == 0);

  immutable_value val2(STRING_TYPE(64), v2);
  ASSERT_TRUE(strcmp(v2, reinterpret_cast<const char *>(val2.ptr())) == 0);

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

#endif /* CONFLUO_TEST_IMMUTABLE_VALUE_TEST_H_ */

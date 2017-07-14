#ifndef TEST_IMMUTABLE_VALUE_TEST_H_
#define TEST_IMMUTABLE_VALUE_TEST_H_

#include "test_utils.h"
#include "gtest/gtest.h"
#include "../dialog/immutable_value.h"

using namespace ::dialog;

class ImmutableValueTest : public testing::Test {
 public:
  // Dummy function to stop compiler from complaining.
  static bool check(bool val) {
    return val;
  }
};

TEST_F(ImmutableValueTest, BoolValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("true", BOOL_TYPE);
  ASSERT_EQ(true, *reinterpret_cast<const bool*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("FALSE", BOOL_TYPE);
  ASSERT_EQ(false, *reinterpret_cast<const bool*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    immutable_value_t::parse("0", BOOL_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    immutable_value_t::parse("true2", BOOL_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    immutable_value_t::parse("t", BOOL_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, CharValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("c", CHAR_TYPE);
  ASSERT_EQ('c', *reinterpret_cast<const char*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("1", CHAR_TYPE);
  ASSERT_EQ('1', *reinterpret_cast<const char*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    immutable_value_t::parse("cc", CHAR_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    immutable_value_t::parse("11", CHAR_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, ShortValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("10", SHORT_TYPE);
  ASSERT_EQ(10, *reinterpret_cast<const short*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("-100", SHORT_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const short*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("10c", SHORT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("1.0", SHORT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("ten", SHORT_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, IntValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("65536", INT_TYPE);
  ASSERT_EQ(65536, *reinterpret_cast<const int*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("-100", INT_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const int*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("10c", INT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("1.0", INT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("ten", INT_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, LongValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("2147483648", LONG_TYPE);
  ASSERT_EQ(2147483648, *reinterpret_cast<const long*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("-100", LONG_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const long*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("10c", LONG_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("1.0", LONG_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("ten", LONG_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, FloatValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("10.4", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(10.4),
            *reinterpret_cast<const float*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("-100.3", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(-100.3),
            *reinterpret_cast<const float*>(val2.data()));

  immutable_value_t val3 = immutable_value_t::parse("-100", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(-100.0),
            *reinterpret_cast<const float*>(val3.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("10.0c", FLOAT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("ten point one", FLOAT_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, DoubleValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("10.4", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(10.4),
            *reinterpret_cast<const double*>(val1.data()));

  immutable_value_t val2 = immutable_value_t::parse("-100.3", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(-100.3),
            *reinterpret_cast<const double*>(val2.data()));

  immutable_value_t val3 = immutable_value_t::parse("-100", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(-100.0),
            *reinterpret_cast<const double*>(val3.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("10.0c", DOUBLE_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    immutable_value_t::parse("ten point one", DOUBLE_TYPE);
  }));

  ASSERT_FALSE(val1 < val2);
  ASSERT_FALSE(val1 <= val2);

  ASSERT_TRUE(val1 > val2);
  ASSERT_TRUE(val1 >= val2);

  ASSERT_FALSE(val1 == val2);
  ASSERT_TRUE(val1 != val2);
}

TEST_F(ImmutableValueTest, StringValueTest) {
  immutable_value_t val1 = immutable_value_t::parse("abc", STRING_TYPE(64));
  ASSERT_TRUE(strcmp("abc", reinterpret_cast<const char*>(val1.data())) == 0);

  immutable_value_t val2 = immutable_value_t::parse("123", STRING_TYPE(64));
  ASSERT_TRUE(strcmp("123", reinterpret_cast<const char*>(val2.data())) == 0);

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    check(val1 < val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    check(val1 <= val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    check(val1 > val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    check(val1 >= val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    check(val1 == val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    check(val1 != val2);
  }));
}

#endif /* TEST_IMMUTABLE_VALUE_TEST_H_ */

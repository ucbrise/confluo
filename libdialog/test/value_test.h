#ifndef TEST_VALUE_TEST_H_
#define TEST_VALUE_TEST_H_

#include "value.h"
#include "test_utils.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class ValueTest : public testing::Test {
 public:
};

TEST_F(ValueTest, BoolValueTest) {
  value_t val1 = value_t::from_string("true", BOOL_TYPE);
  ASSERT_EQ(true, *reinterpret_cast<const bool*>(val1.data()));

  value_t val2 = value_t::from_string("FALSE", BOOL_TYPE);
  ASSERT_EQ(false, *reinterpret_cast<const bool*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    value_t::from_string("0", BOOL_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    value_t::from_string("true2", BOOL_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    value_t::from_string("t", BOOL_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, CharValueTest) {
  value_t val1 = value_t::from_string("c", CHAR_TYPE);
  ASSERT_EQ('c', *reinterpret_cast<const char*>(val1.data()));

  value_t val2 = value_t::from_string("1", CHAR_TYPE);
  ASSERT_EQ('1', *reinterpret_cast<const char*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    value_t::from_string("cc", CHAR_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    value_t::from_string("11", CHAR_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, ShortValueTest) {
  value_t val1 = value_t::from_string("10", SHORT_TYPE);
  ASSERT_EQ(10, *reinterpret_cast<const short*>(val1.data()));

  value_t val2 = value_t::from_string("-100", SHORT_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const short*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("10c", SHORT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("1.0", SHORT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("ten", SHORT_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, IntValueTest) {
  value_t val1 = value_t::from_string("65536", INT_TYPE);
  ASSERT_EQ(65536, *reinterpret_cast<const int*>(val1.data()));

  value_t val2 = value_t::from_string("-100", INT_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const int*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("10c", INT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("1.0", INT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("ten", INT_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, LongValueTest) {
  value_t val1 = value_t::from_string("2147483648", LONG_TYPE);
  ASSERT_EQ(2147483648, *reinterpret_cast<const long*>(val1.data()));

  value_t val2 = value_t::from_string("-100", LONG_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const long*>(val2.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("10c", LONG_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("1.0", LONG_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("ten", LONG_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, FloatValueTest) {
  value_t val1 = value_t::from_string("10.4", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(10.4), *reinterpret_cast<const float*>(val1.data()));

  value_t val2 = value_t::from_string("-100.3", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(-100.3), *reinterpret_cast<const float*>(val2.data()));

  value_t val3 = value_t::from_string("-100", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(-100.0), *reinterpret_cast<const float*>(val3.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("10.0c", FLOAT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("ten point one", FLOAT_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, DoubleValueTest) {
  value_t val1 = value_t::from_string("10.4", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(10.4), *reinterpret_cast<const double*>(val1.data()));

  value_t val2 = value_t::from_string("-100.3", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(-100.3), *reinterpret_cast<const double*>(val2.data()));

  value_t val3 = value_t::from_string("-100", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(-100.0), *reinterpret_cast<const double*>(val3.data()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("10.0c", DOUBLE_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    value_t::from_string("ten point one", DOUBLE_TYPE);
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, StringValueTest) {
  value_t val1 = value_t::from_string("abc", STRING_TYPE(64));
  ASSERT_TRUE(strcmp("abc", reinterpret_cast<const char*>(val1.data())) == 0);

  value_t val2 = value_t::from_string("123", STRING_TYPE(64));
  ASSERT_TRUE(strcmp("123", reinterpret_cast<const char*>(val2.data())) == 0);

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    val1.relop(relop_id::LT, val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    val1.relop(relop_id::LE, val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    val1.relop(relop_id::GT, val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    val1.relop(relop_id::GE, val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    val1.relop(relop_id::EQ, val2);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([&val1, &val2]() {
    val1.relop(relop_id::NEQ, val2);
  }));
}

#endif /* TEST_VALUE_TEST_H_ */

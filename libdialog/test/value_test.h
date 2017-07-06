#ifndef TEST_VALUE_TEST_H_
#define TEST_VALUE_TEST_H_

#include "value.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class ValueTest : public testing::Test {
 public:
  template<typename F>
  bool test_fail(F f) {
    bool fail = false;
    try {
      f();
    } catch (std::exception& ex) {
      fail = true;
    }
    return fail;
  }
};

TEST_F(ValueTest, BoolValueTest) {
  value_t val1 = value_t::from_string("true", bool_type());
  ASSERT_EQ(true, *reinterpret_cast<bool*>(val1.data));

  value_t val2 = value_t::from_string("FALSE", bool_type());
  ASSERT_EQ(false, *reinterpret_cast<bool*>(val2.data));

  ASSERT_TRUE(test_fail([]() {
    value_t::from_string("0", bool_type());
  }));

  ASSERT_TRUE(test_fail([]() {
    value_t::from_string("true2", bool_type());
  }));

  ASSERT_TRUE(test_fail([]() {
    value_t::from_string("t", bool_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, CharValueTest) {
  value_t val1 = value_t::from_string("c", char_type());
  ASSERT_EQ('c', *reinterpret_cast<char*>(val1.data));

  value_t val2 = value_t::from_string("1", char_type());
  ASSERT_EQ('1', *reinterpret_cast<char*>(val2.data));

  ASSERT_TRUE(test_fail([]() {
    value_t::from_string("cc", char_type());
  }));

  ASSERT_TRUE(test_fail([]() {
    value_t::from_string("11", char_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, ShortValueTest) {
  value_t val1 = value_t::from_string("10", short_type());
  ASSERT_EQ(10, *reinterpret_cast<short*>(val1.data));

  value_t val2 = value_t::from_string("-100", short_type());
  ASSERT_EQ(-100, *reinterpret_cast<short*>(val2.data));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("10c", short_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("1.0", short_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("ten", short_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, IntValueTest) {
  value_t val1 = value_t::from_string("65536", int_type());
  ASSERT_EQ(65536, *reinterpret_cast<int*>(val1.data));

  value_t val2 = value_t::from_string("-100", int_type());
  ASSERT_EQ(-100, *reinterpret_cast<int*>(val2.data));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("10c", int_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("1.0", int_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("ten", int_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, LongValueTest) {
  value_t val1 = value_t::from_string("2147483648", long_type());
  ASSERT_EQ(2147483648, *reinterpret_cast<long*>(val1.data));

  value_t val2 = value_t::from_string("-100", long_type());
  ASSERT_EQ(-100, *reinterpret_cast<long*>(val2.data));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("10c", long_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("1.0", long_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("ten", long_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, FloatValueTest) {
  value_t val1 = value_t::from_string("10.4", float_type());
  ASSERT_EQ(static_cast<float>(10.4), *reinterpret_cast<float*>(val1.data));

  value_t val2 = value_t::from_string("-100.3", float_type());
  ASSERT_EQ(static_cast<float>(-100.3), *reinterpret_cast<float*>(val2.data));

  value_t val3 = value_t::from_string("-100", float_type());
  ASSERT_EQ(static_cast<float>(-100.0), *reinterpret_cast<float*>(val3.data));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("10.0c", float_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("ten point one", float_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, DoubleValueTest) {
  value_t val1 = value_t::from_string("10.4", double_type());
  ASSERT_EQ(static_cast<double>(10.4), *reinterpret_cast<double*>(val1.data));

  value_t val2 = value_t::from_string("-100.3", double_type());
  ASSERT_EQ(static_cast<double>(-100.3), *reinterpret_cast<double*>(val2.data));

  value_t val3 = value_t::from_string("-100", double_type());
  ASSERT_EQ(static_cast<double>(-100.0), *reinterpret_cast<double*>(val3.data));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("10.0c", double_type());
  }));

  ASSERT_TRUE(test_fail([] {
    value_t::from_string("ten point one", double_type());
  }));

  ASSERT_FALSE(val1.relop(relop_id::LT, val2));
  ASSERT_FALSE(val1.relop(relop_id::LE, val2));

  ASSERT_TRUE(val1.relop(relop_id::GT, val2));
  ASSERT_TRUE(val1.relop(relop_id::GE, val2));

  ASSERT_FALSE(val1.relop(relop_id::EQ, val2));
  ASSERT_TRUE(val1.relop(relop_id::NEQ, val2));
}

TEST_F(ValueTest, StringValueTest) {
  value_t val1 = value_t::from_string("abc", string_type(64));
  ASSERT_TRUE(strcmp("abc", reinterpret_cast<char*>(val1.data)) == 0);

  value_t val2 = value_t::from_string("123", string_type(64));
  ASSERT_TRUE(strcmp("123", reinterpret_cast<char*>(val2.data)) == 0);

  ASSERT_TRUE(test_fail([&val1, &val2]() {
    val1.relop(relop_id::LT, val2);
  }));

  ASSERT_TRUE(test_fail([&val1, &val2]() {
    val1.relop(relop_id::LE, val2);
  }));

  ASSERT_TRUE(test_fail([&val1, &val2]() {
    val1.relop(relop_id::GT, val2);
  }));

  ASSERT_TRUE(test_fail([&val1, &val2]() {
    val1.relop(relop_id::GE, val2);
  }));

  ASSERT_TRUE(test_fail([&val1, &val2]() {
    val1.relop(relop_id::EQ, val2);
  }));

  ASSERT_TRUE(test_fail([&val1, &val2]() {
    val1.relop(relop_id::NEQ, val2);
  }));
}

#endif /* TEST_VALUE_TEST_H_ */

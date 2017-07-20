#ifndef TEST_MUTABLE_VALUE_TEST_H_
#define TEST_MUTABLE_VALUE_TEST_H_

#include <cmath>

#include "test_utils.h"
#include "gtest/gtest.h"
#include "mutable_value.h"

using namespace ::dialog;

class MutableValueTest : public testing::Test {
 public:
  template<typename T>
  bool approx_equals(const mutable_value& a, const mutable_value& b) {
    T af = *reinterpret_cast<const T*>(a.ptr());
    T bf = *reinterpret_cast<const T*>(b.ptr());
    return fabs(af - bf) < 0.00001;
  }
};

TEST_F(MutableValueTest, BoolValueTest) {
  mutable_value n1(true), n2(false), n3(true);
  ASSERT_EQ(n1.type().id, type_id::D_BOOL);
  ASSERT_EQ(n2.type().id, type_id::D_BOOL);
  ASSERT_EQ(n3.type().id, type_id::D_BOOL);

  ASSERT_EQ(*reinterpret_cast<const bool*>(n1.ptr()), true);
  ASSERT_EQ(*reinterpret_cast<const bool*>(n2.ptr()), false);
  ASSERT_EQ(*reinterpret_cast<const bool*>(n3.ptr()), true);

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(true) == (n1 + n2));
  ASSERT_TRUE(mutable_value(true) == (n1 - n2));
  ASSERT_TRUE(mutable_value(false) == (n1 * n2));
  ASSERT_TRUE(mutable_value(false) == (n2 / n1));
  ASSERT_TRUE(mutable_value(false) == (n2 % n1));
}

TEST_F(MutableValueTest, CharValueTest) {

  mutable_value n1(static_cast<char>(5)), n2(static_cast<char>(2)), n3(
      static_cast<char>(5));
  ASSERT_EQ(n1.type().id, type_id::D_CHAR);
  ASSERT_EQ(n2.type().id, type_id::D_CHAR);
  ASSERT_EQ(n3.type().id, type_id::D_CHAR);

  ASSERT_EQ(*reinterpret_cast<const char*>(n1.ptr()), static_cast<char>(5));
  ASSERT_EQ(*reinterpret_cast<const char*>(n2.ptr()), static_cast<char>(2));
  ASSERT_EQ(*reinterpret_cast<const char*>(n3.ptr()), static_cast<char>(5));

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(static_cast<char>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value(static_cast<char>(5)) == (+n1));
  ASSERT_TRUE(mutable_value(static_cast<char>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value(static_cast<char>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value(static_cast<char>(5 >> 2)) == (n1 >> n2));
}

TEST_F(MutableValueTest, ShortValueTest) {

  mutable_value n1(static_cast<short>(5)), n2(static_cast<short>(2)), n3(
      static_cast<short>(5));
  ASSERT_EQ(n1.type().id, type_id::D_SHORT);
  ASSERT_EQ(n2.type().id, type_id::D_SHORT);
  ASSERT_EQ(n3.type().id, type_id::D_SHORT);

  ASSERT_EQ(*reinterpret_cast<const short*>(n1.ptr()), static_cast<short>(5));
  ASSERT_EQ(*reinterpret_cast<const short*>(n2.ptr()), static_cast<short>(2));
  ASSERT_EQ(*reinterpret_cast<const short*>(n3.ptr()), static_cast<short>(5));

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(static_cast<short>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value(static_cast<short>(5)) == (+n1));
  ASSERT_TRUE(mutable_value(static_cast<short>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value(static_cast<short>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value(static_cast<short>(5 >> 2)) == (n1 >> n2));
}

TEST_F(MutableValueTest, IntValueTest) {
  mutable_value n1(static_cast<int>(5)), n2(static_cast<int>(2)), n3(
      static_cast<int>(5));
  ASSERT_EQ(n1.type().id, type_id::D_INT);
  ASSERT_EQ(n2.type().id, type_id::D_INT);
  ASSERT_EQ(n3.type().id, type_id::D_INT);

  ASSERT_EQ(*reinterpret_cast<const int*>(n1.ptr()), static_cast<int>(5));
  ASSERT_EQ(*reinterpret_cast<const int*>(n2.ptr()), static_cast<int>(2));
  ASSERT_EQ(*reinterpret_cast<const int*>(n3.ptr()), static_cast<int>(5));

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(static_cast<int>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value(static_cast<int>(5)) == (+n1));
  ASSERT_TRUE(mutable_value(static_cast<int>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value(static_cast<int>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value(static_cast<int>(5 >> 2)) == (n1 >> n2));
}

TEST_F(MutableValueTest, LongValueTest) {
  mutable_value n1(static_cast<long>(5)), n2(static_cast<long>(2)), n3(
      static_cast<long>(5));
  ASSERT_EQ(n1.type().id, type_id::D_LONG);
  ASSERT_EQ(n2.type().id, type_id::D_LONG);
  ASSERT_EQ(n3.type().id, type_id::D_LONG);

  ASSERT_EQ(*reinterpret_cast<const long*>(n1.ptr()), static_cast<long>(5));
  ASSERT_EQ(*reinterpret_cast<const long*>(n2.ptr()), static_cast<long>(2));
  ASSERT_EQ(*reinterpret_cast<const long*>(n3.ptr()), static_cast<long>(5));

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(static_cast<long>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value(static_cast<long>(5)) == (+n1));
  ASSERT_TRUE(mutable_value(static_cast<long>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value(static_cast<long>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value(static_cast<long>(5 >> 2)) == (n1 >> n2));
}

TEST_F(MutableValueTest, FloatValueTest) {
  mutable_value n1(static_cast<float>(5.2)), n2(static_cast<float>(2.6)), n3(
      static_cast<float>(5.2));
  ASSERT_EQ(n1.type().id, type_id::D_FLOAT);
  ASSERT_EQ(n2.type().id, type_id::D_FLOAT);
  ASSERT_EQ(n3.type().id, type_id::D_FLOAT);

  ASSERT_EQ(*reinterpret_cast<const float*>(n1.ptr()), static_cast<float>(5.2));
  ASSERT_EQ(*reinterpret_cast<const float*>(n2.ptr()), static_cast<float>(2.6));
  ASSERT_EQ(*reinterpret_cast<const float*>(n3.ptr()), static_cast<float>(5.2));

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(static_cast<float>(-5.2)) == (-n1));
  ASSERT_TRUE(mutable_value(static_cast<float>(5.2)) == (+n1));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value(static_cast<float>(7.8)),
                           (n1 + n2)));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value(static_cast<float>(2.6)),
                           (n1 - n2)));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value(static_cast<float>(13.52)),
                           (n1 * n2)));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value(static_cast<float>(2.0)),
                           (n1 / n2)));

  test::test_utils::test_fail([&n1]() {(~n1);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 & n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 | n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 ^ n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 << n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 >> n2);});
}

TEST_F(MutableValueTest, DoubleValueTest) {
  mutable_value n1(static_cast<double>(5.2)), n2(static_cast<double>(2.6)),
      n3(static_cast<double>(5.2));
  ASSERT_EQ(n1.type().id, type_id::D_DOUBLE);
  ASSERT_EQ(n2.type().id, type_id::D_DOUBLE);
  ASSERT_EQ(n3.type().id, type_id::D_DOUBLE);

  ASSERT_EQ(*reinterpret_cast<const double*>(n1.ptr()),
            static_cast<double>(5.2));
  ASSERT_EQ(*reinterpret_cast<const double*>(n2.ptr()),
            static_cast<double>(2.6));
  ASSERT_EQ(*reinterpret_cast<const double*>(n3.ptr()),
            static_cast<double>(5.2));

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  ASSERT_TRUE(mutable_value(static_cast<double>(-5.2)) == (-n1));
  ASSERT_TRUE(mutable_value(static_cast<double>(5.2)) == (+n1));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value(static_cast<double>(7.8)),
                            (n1 + n2)));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value(static_cast<double>(2.6)),
                            (n1 - n2)));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value(static_cast<double>(13.52)),
                            (n1 * n2)));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value(static_cast<double>(2.0)),
                            (n1 / n2)));

  test::test_utils::test_fail([&n1]() {(~n1);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 & n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 | n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 ^ n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 << n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 >> n2);});
}

TEST_F(MutableValueTest, StringValueTest) {
  mutable_value n1(std::string("abc")), n2(std::string("123")), n3(
      std::string("abc"));
  ASSERT_EQ(n1.type().id, type_id::D_STRING);
  ASSERT_EQ(n2.type().id, type_id::D_STRING);
  ASSERT_EQ(n3.type().id, type_id::D_STRING);

  ASSERT_TRUE(strcmp(reinterpret_cast<const char*>(n1.ptr()), "abc") == 0);
  ASSERT_TRUE(strcmp(reinterpret_cast<const char*>(n2.ptr()), "123") == 0);
  ASSERT_TRUE(strcmp(reinterpret_cast<const char*>(n3.ptr()), "abc") == 0);

  ASSERT_TRUE(n1 == n3);
  ASSERT_TRUE(n1 != n2);
  ASSERT_TRUE(n1 > n2);
  ASSERT_TRUE(n1 >= n2);
  ASSERT_TRUE(n2 < n1);
  ASSERT_TRUE(n2 <= n1);

  ASSERT_FALSE(n1 == n2);
  ASSERT_FALSE(n1 != n3);
  ASSERT_FALSE(n2 > n1);
  ASSERT_FALSE(n2 >= n1);
  ASSERT_FALSE(n1 < n2);
  ASSERT_FALSE(n1 <= n2);

  test::test_utils::test_fail([&n1]() {(-n1);});
  test::test_utils::test_fail([&n1]() {(+n1);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 + n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 - n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 * n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 / n2);});
  test::test_utils::test_fail([&n1]() {(~n1);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 & n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 | n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 ^ n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 << n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 >> n2);});
}

TEST_F(MutableValueTest, BoolParseTest) {
  mutable_value val1 = mutable_value::parse("true", BOOL_TYPE);
  ASSERT_EQ(true, *reinterpret_cast<const bool*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("FALSE", BOOL_TYPE);
  ASSERT_EQ(false, *reinterpret_cast<const bool*>(val2.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("0", BOOL_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("true2", BOOL_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("t", BOOL_TYPE);
  }));
}

TEST_F(MutableValueTest, CharParseTest) {
  mutable_value val1 = mutable_value::parse("c", CHAR_TYPE);
  ASSERT_EQ('c', *reinterpret_cast<const char*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("1", CHAR_TYPE);
  ASSERT_EQ('1', *reinterpret_cast<const char*>(val2.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("cc", CHAR_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("11", CHAR_TYPE);
  }));
}

TEST_F(MutableValueTest, ShortParseTest) {
  mutable_value val1 = mutable_value::parse("10", SHORT_TYPE);
  ASSERT_EQ(10, *reinterpret_cast<const short*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("-100", SHORT_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const short*>(val2.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("10c", SHORT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("1.0", SHORT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("ten", SHORT_TYPE);
  }));
}

TEST_F(MutableValueTest, IntParseTest) {
  mutable_value val1 = mutable_value::parse("65536", INT_TYPE);
  ASSERT_EQ(65536, *reinterpret_cast<const int*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("-100", INT_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const int*>(val2.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("10c", INT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("1.0", INT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("ten", INT_TYPE);
  }));
}

TEST_F(MutableValueTest, LongParseTest) {
  mutable_value val1 = mutable_value::parse("2147483648", LONG_TYPE);
  ASSERT_EQ(2147483648, *reinterpret_cast<const long*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("-100", LONG_TYPE);
  ASSERT_EQ(-100, *reinterpret_cast<const long*>(val2.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("10c", LONG_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("1.0", LONG_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("ten", LONG_TYPE);
  }));
}

TEST_F(MutableValueTest, FloatParseTest) {
  mutable_value val1 = mutable_value::parse("10.4", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(10.4),
            *reinterpret_cast<const float*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("-100.3", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(-100.3),
            *reinterpret_cast<const float*>(val2.ptr()));

  mutable_value val3 = mutable_value::parse("-100", FLOAT_TYPE);
  ASSERT_EQ(static_cast<float>(-100.0),
            *reinterpret_cast<const float*>(val3.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("10.0c", FLOAT_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("ten point one", FLOAT_TYPE);
  }));
}

TEST_F(MutableValueTest, DoubleParseTest) {
  mutable_value val1 = mutable_value::parse("10.4", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(10.4),
            *reinterpret_cast<const double*>(val1.ptr()));

  mutable_value val2 = mutable_value::parse("-100.3", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(-100.3),
            *reinterpret_cast<const double*>(val2.ptr()));

  mutable_value val3 = mutable_value::parse("-100", DOUBLE_TYPE);
  ASSERT_EQ(static_cast<double>(-100.0),
            *reinterpret_cast<const double*>(val3.ptr()));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("10.0c", DOUBLE_TYPE);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([] {
    mutable_value::parse("ten point one", DOUBLE_TYPE);
  }));
}

TEST_F(MutableValueTest, StringParseTest) {
  mutable_value val1 = mutable_value::parse("abc", STRING_TYPE(64));
  ASSERT_TRUE(strcmp("abc", reinterpret_cast<const char*>(val1.ptr())) == 0);

  mutable_value val2 = mutable_value::parse("123", STRING_TYPE(64));
  ASSERT_TRUE(strcmp("123", reinterpret_cast<const char*>(val2.ptr())) == 0);
}

#endif /* TEST_MUTABLE_VALUE_TEST_H_ */

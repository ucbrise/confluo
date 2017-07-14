#ifndef TEST_MUTABLE_VALUE_TEST_H_
#define TEST_MUTABLE_VALUE_TEST_H_

#include <cmath>

#include "test_utils.h"
#include "gtest/gtest.h"
#include "../dialog/mutable_value.h"

using namespace ::dialog;

class NumericTest : public testing::Test {
 public:
  template<typename T>
  bool approx_equals(const mutable_value_t& a, const mutable_value_t& b) {
    T af = *reinterpret_cast<const T*>(a.data());
    T bf = *reinterpret_cast<const T*>(b.data());
    return fabs(af - bf) < 0.00001;
  }
};

TEST_F(NumericTest, BoolNumericTest) {
  mutable_value_t n1(true), n2(false), n3(true);
  ASSERT_EQ(n1.type().id, type_id::D_BOOL);
  ASSERT_EQ(n2.type().id, type_id::D_BOOL);
  ASSERT_EQ(n3.type().id, type_id::D_BOOL);

  ASSERT_EQ(*reinterpret_cast<const bool*>(n1.data()), true);
  ASSERT_EQ(*reinterpret_cast<const bool*>(n2.data()), false);
  ASSERT_EQ(*reinterpret_cast<const bool*>(n3.data()), true);

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

  ASSERT_TRUE(mutable_value_t(true) == (n1 + n2));
  ASSERT_TRUE(mutable_value_t(true) == (n1 - n2));
  ASSERT_TRUE(mutable_value_t(false) == (n1 * n2));
  ASSERT_TRUE(mutable_value_t(false) == (n2 / n1));
  ASSERT_TRUE(mutable_value_t(false) == (n2 % n1));
}

TEST_F(NumericTest, CharNumericTest) {

  mutable_value_t n1(static_cast<char>(5)), n2(static_cast<char>(2)), n3(
      static_cast<char>(5));
  ASSERT_EQ(n1.type().id, type_id::D_CHAR);
  ASSERT_EQ(n2.type().id, type_id::D_CHAR);
  ASSERT_EQ(n3.type().id, type_id::D_CHAR);

  ASSERT_EQ(*reinterpret_cast<const char*>(n1.data()),
            static_cast<char>(5));
  ASSERT_EQ(*reinterpret_cast<const char*>(n2.data()),
            static_cast<char>(2));
  ASSERT_EQ(*reinterpret_cast<const char*>(n3.data()),
            static_cast<char>(5));

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

  ASSERT_TRUE(mutable_value_t(static_cast<char>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(5)) == (+n1));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value_t(static_cast<char>(5 >> 2)) == (n1 >> n2));
}

TEST_F(NumericTest, ShortNumericTest) {

  mutable_value_t n1(static_cast<short>(5)), n2(static_cast<short>(2)), n3(
      static_cast<short>(5));
  ASSERT_EQ(n1.type().id, type_id::D_SHORT);
  ASSERT_EQ(n2.type().id, type_id::D_SHORT);
  ASSERT_EQ(n3.type().id, type_id::D_SHORT);

  ASSERT_EQ(*reinterpret_cast<const short*>(n1.data()),
            static_cast<short>(5));
  ASSERT_EQ(*reinterpret_cast<const short*>(n2.data()),
            static_cast<short>(2));
  ASSERT_EQ(*reinterpret_cast<const short*>(n3.data()),
            static_cast<short>(5));

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

  ASSERT_TRUE(mutable_value_t(static_cast<short>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(5)) == (+n1));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value_t(static_cast<short>(5 >> 2)) == (n1 >> n2));
}

TEST_F(NumericTest, IntNumericTest) {
  mutable_value_t n1(static_cast<int>(5)), n2(static_cast<int>(2)), n3(
      static_cast<int>(5));
  ASSERT_EQ(n1.type().id, type_id::D_INT);
  ASSERT_EQ(n2.type().id, type_id::D_INT);
  ASSERT_EQ(n3.type().id, type_id::D_INT);

  ASSERT_EQ(*reinterpret_cast<const int*>(n1.data()), static_cast<int>(5));
  ASSERT_EQ(*reinterpret_cast<const int*>(n2.data()), static_cast<int>(2));
  ASSERT_EQ(*reinterpret_cast<const int*>(n3.data()), static_cast<int>(5));

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

  ASSERT_TRUE(mutable_value_t(static_cast<int>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(5)) == (+n1));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value_t(static_cast<int>(5 >> 2)) == (n1 >> n2));
}

TEST_F(NumericTest, LongNumericTest) {
  mutable_value_t n1(static_cast<long>(5)), n2(static_cast<long>(2)), n3(
      static_cast<long>(5));
  ASSERT_EQ(n1.type().id, type_id::D_LONG);
  ASSERT_EQ(n2.type().id, type_id::D_LONG);
  ASSERT_EQ(n3.type().id, type_id::D_LONG);

  ASSERT_EQ(*reinterpret_cast<const long*>(n1.data()),
            static_cast<long>(5));
  ASSERT_EQ(*reinterpret_cast<const long*>(n2.data()),
            static_cast<long>(2));
  ASSERT_EQ(*reinterpret_cast<const long*>(n3.data()),
            static_cast<long>(5));

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

  ASSERT_TRUE(mutable_value_t(static_cast<long>(-5)) == (-n1));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(5)) == (+n1));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(7)) == (n1 + n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(3)) == (n1 - n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(10)) == (n1 * n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(2)) == (n1 / n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(1)) == (n1 % n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(~5)) == (~n1));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(5 & 2)) == (n1 & n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(5 | 2)) == (n1 | n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(5 ^ 2)) == (n1 ^ n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(5 << 2)) == (n1 << n2));
  ASSERT_TRUE(mutable_value_t(static_cast<long>(5 >> 2)) == (n1 >> n2));
}

TEST_F(NumericTest, FloatNumericTest) {
  mutable_value_t n1(static_cast<float>(5.2)), n2(static_cast<float>(2.6)), n3(
      static_cast<float>(5.2));
  ASSERT_EQ(n1.type().id, type_id::D_FLOAT);
  ASSERT_EQ(n2.type().id, type_id::D_FLOAT);
  ASSERT_EQ(n3.type().id, type_id::D_FLOAT);

  ASSERT_EQ(*reinterpret_cast<const float*>(n1.data()),
            static_cast<float>(5.2));
  ASSERT_EQ(*reinterpret_cast<const float*>(n2.data()),
            static_cast<float>(2.6));
  ASSERT_EQ(*reinterpret_cast<const float*>(n3.data()),
            static_cast<float>(5.2));

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

  ASSERT_TRUE(mutable_value_t(static_cast<float>(-5.2)) == (-n1));
  ASSERT_TRUE(mutable_value_t(static_cast<float>(5.2)) == (+n1));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value_t(static_cast<float>(7.8)), (n1 + n2)));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value_t(static_cast<float>(2.6)), (n1 - n2)));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value_t(static_cast<float>(13.52)), (n1 * n2)));
  ASSERT_TRUE(
      approx_equals<float>(mutable_value_t(static_cast<float>(2.0)), (n1 / n2)));

  test::test_utils::test_fail([&n1]() {(~n1);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 & n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 | n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 ^ n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 << n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 >> n2);});
}

TEST_F(NumericTest, DoubleNumericTest) {
  mutable_value_t n1(static_cast<double>(5.2)), n2(static_cast<double>(2.6)), n3(
      static_cast<double>(5.2));
  ASSERT_EQ(n1.type().id, type_id::D_DOUBLE);
  ASSERT_EQ(n2.type().id, type_id::D_DOUBLE);
  ASSERT_EQ(n3.type().id, type_id::D_DOUBLE);

  ASSERT_EQ(*reinterpret_cast<const double*>(n1.data()),
            static_cast<double>(5.2));
  ASSERT_EQ(*reinterpret_cast<const double*>(n2.data()),
            static_cast<double>(2.6));
  ASSERT_EQ(*reinterpret_cast<const double*>(n3.data()),
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

  ASSERT_TRUE(mutable_value_t(static_cast<double>(-5.2)) == (-n1));
  ASSERT_TRUE(mutable_value_t(static_cast<double>(5.2)) == (+n1));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value_t(static_cast<double>(7.8)), (n1 + n2)));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value_t(static_cast<double>(2.6)), (n1 - n2)));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value_t(static_cast<double>(13.52)), (n1 * n2)));
  ASSERT_TRUE(
      approx_equals<double>(mutable_value_t(static_cast<double>(2.0)), (n1 / n2)));

  test::test_utils::test_fail([&n1]() {(~n1);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 & n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 | n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 ^ n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 << n2);});
  test::test_utils::test_fail([&n1, &n2]() {(n1 >> n2);});
}

#endif /* TEST_MUTABLE_VALUE_TEST_H_ */

#ifndef TEST_DATA_TYPES_TEST_H_
#define TEST_DATA_TYPES_TEST_H_

#include "data_types.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class DataTypesTest : public testing::Test {
};

TEST_F(DataTypesTest, GetterTest) {
  data_type t1 = BOOL_TYPE, t2 = CHAR_TYPE, t3 = SHORT_TYPE, t4 = INT_TYPE, t5 =
      LONG_TYPE, t6 = FLOAT_TYPE, t7 = DOUBLE_TYPE, t8 = NONE_TYPE;

  ASSERT_TRUE(BOOL_TYPE == t1);
  ASSERT_TRUE(CHAR_TYPE == t2);
  ASSERT_TRUE(SHORT_TYPE == t3);
  ASSERT_TRUE(INT_TYPE == t4);
  ASSERT_TRUE(LONG_TYPE == t5);
  ASSERT_TRUE(FLOAT_TYPE == t6);
  ASSERT_TRUE(DOUBLE_TYPE == t7);
  ASSERT_TRUE(NONE_TYPE == t8);

  ASSERT_TRUE(BOOL_TYPE != CHAR_TYPE);

  ASSERT_TRUE(t1.to_string() == "bool");
  ASSERT_TRUE(t2.to_string() == "char");
  ASSERT_TRUE(t3.to_string() == "short");
  ASSERT_TRUE(t4.to_string() == "int");
  ASSERT_TRUE(t5.to_string() == "long");
  ASSERT_TRUE(t6.to_string() == "float");
  ASSERT_TRUE(t7.to_string() == "double");
  ASSERT_TRUE(t8.to_string() == "none");
}

#endif /* TEST_DATA_TYPES_TEST_H_ */

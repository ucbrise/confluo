#ifndef TEST_DATA_TYPES_TEST_H_
#define TEST_DATA_TYPES_TEST_H_

#include "types/data_types.h"

#include "gtest/gtest.h"

using namespace ::confluo;

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

  ASSERT_TRUE(t1.name() == "bool");
  ASSERT_TRUE(t2.name() == "char");
  ASSERT_TRUE(t3.name() == "short");
  ASSERT_TRUE(t4.name() == "int");
  ASSERT_TRUE(t5.name() == "long");
  ASSERT_TRUE(t6.name() == "float");
  ASSERT_TRUE(t7.name() == "double");
  ASSERT_TRUE(t8.name() == "none");
}

#endif /* TEST_DATA_TYPES_TEST_H_ */

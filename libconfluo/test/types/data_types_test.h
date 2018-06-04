#ifndef CONFLUO_TEST_DATA_TYPES_TEST_H_
#define CONFLUO_TEST_DATA_TYPES_TEST_H_

#include "gtest/gtest.h"
#include "types/data_type.h"

using namespace ::confluo;

class DataTypesTest : public testing::Test {
};

TEST_F(DataTypesTest, GetterTest) {
  data_type t1 = primitive_types::BOOL_TYPE(), t2 = primitive_types::CHAR_TYPE(), t3 = primitive_types::SHORT_TYPE(),
      t4 = primitive_types::INT_TYPE(), t5 = primitive_types::LONG_TYPE(), t6 = primitive_types::FLOAT_TYPE(),
      t7 = primitive_types::DOUBLE_TYPE(), t8 = primitive_types::NONE_TYPE();

  ASSERT_TRUE(primitive_types::BOOL_TYPE() == t1);
  ASSERT_TRUE(primitive_types::CHAR_TYPE() == t2);
  ASSERT_TRUE(primitive_types::SHORT_TYPE() == t3);
  ASSERT_TRUE(primitive_types::INT_TYPE() == t4);
  ASSERT_TRUE(primitive_types::LONG_TYPE() == t5);
  ASSERT_TRUE(primitive_types::FLOAT_TYPE() == t6);
  ASSERT_TRUE(primitive_types::DOUBLE_TYPE() == t7);
  ASSERT_TRUE(primitive_types::NONE_TYPE() == t8);

  ASSERT_TRUE(primitive_types::BOOL_TYPE() != primitive_types::CHAR_TYPE());

  ASSERT_TRUE(t1.name() == "bool");
  ASSERT_TRUE(t2.name() == "char");
  ASSERT_TRUE(t3.name() == "short");
  ASSERT_TRUE(t4.name() == "int");
  ASSERT_TRUE(t5.name() == "long");
  ASSERT_TRUE(t6.name() == "float");
  ASSERT_TRUE(t7.name() == "double");
  ASSERT_TRUE(t8.name() == "none");
}

#endif /* CONFLUO_TEST_DATA_TYPES_TEST_H_ */

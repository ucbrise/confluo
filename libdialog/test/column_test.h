#ifndef TEST_COLUMN_TEST_H_
#define TEST_COLUMN_TEST_H_

#include "column.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class ColumnTest : public testing::Test {
};

TEST_F(ColumnTest, GetterTest) {
  column_t col(0, 0, int_type(), "test");

  ASSERT_EQ(0, col.idx());
  ASSERT_EQ(0, col.offset());
  ASSERT_EQ("TEST", col.name());
  ASSERT_TRUE(INT_TYPE == col.type());
  ASSERT_FALSE(col.is_indexed());
}

TEST_F(ColumnTest, IndexStateTest) {
  column_t col(0, 0, int_type(), "test");
  ASSERT_FALSE(col.is_indexed());

  bool success = col.set_indexing();
  ASSERT_TRUE(success);

  col.set_indexed(3);
  ASSERT_TRUE(col.is_indexed());
  ASSERT_EQ(3, col.index_id());

  col.set_unindexed();
  ASSERT_FALSE(col.is_indexed());
}

#endif /* TEST_COLUMN_TEST_H_ */

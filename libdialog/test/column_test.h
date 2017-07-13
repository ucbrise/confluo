#ifndef TEST_COLUMN_TEST_H_
#define TEST_COLUMN_TEST_H_

#include "column.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class ColumnTest : public testing::Test {
};

TEST_F(ColumnTest, GetterTest) {
  column_t col(0, 0, INT_TYPE, "test", numeric_t(INT_TYPE, INT_TYPE.min()),
               numeric_t(INT_TYPE, INT_TYPE.max()));

  ASSERT_EQ(0, col.idx());
  ASSERT_EQ(0, col.offset());
  ASSERT_EQ("TEST", col.name());
  ASSERT_TRUE(INT_TYPE == col.type());
  ASSERT_FALSE(col.is_indexed());
}

TEST_F(ColumnTest, IndexStateTest) {
  column_t col(0, 0, INT_TYPE, "test", numeric_t(INT_TYPE, INT_TYPE.min()),
               numeric_t(INT_TYPE, INT_TYPE.max()));
  ASSERT_FALSE(col.is_indexed());

  bool success = col.set_indexing();
  ASSERT_TRUE(success);

  col.set_indexed(3, 0.1);
  ASSERT_TRUE(col.is_indexed());
  ASSERT_EQ(3, col.index_id());
  ASSERT_EQ(static_cast<double>(0.1), col.index_bucket_size());

  col.set_unindexed();
  ASSERT_FALSE(col.is_indexed());
}

#endif /* TEST_COLUMN_TEST_H_ */

#ifndef CONFLUO_TEST_COLUMN_TEST_H_
#define CONFLUO_TEST_COLUMN_TEST_H_

#include "schema/column.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class ColumnTest : public testing::Test {
};

TEST_F(ColumnTest, GetterTest) {
  column_t col(0,
               0,
               primitive_types::INT_TYPE(),
               "test",
               mutable_value(primitive_types::INT_TYPE(), primitive_types::INT_TYPE().min()),
               mutable_value(primitive_types::INT_TYPE(), primitive_types::INT_TYPE().max()));

  ASSERT_EQ(0, col.idx());
  ASSERT_EQ(0, col.offset());
  ASSERT_EQ("TEST", col.name());
  ASSERT_TRUE(primitive_types::INT_TYPE() == col.type());
  ASSERT_FALSE(col.is_indexed());
}

TEST_F(ColumnTest, IndexStateTest) {
  column_t col(0,
               0,
               primitive_types::INT_TYPE(),
               "test",
               mutable_value(primitive_types::INT_TYPE(), primitive_types::INT_TYPE().min()),
               mutable_value(primitive_types::INT_TYPE(), primitive_types::INT_TYPE().max()));
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

#endif /* CONFLUO_TEST_COLUMN_TEST_H_ */

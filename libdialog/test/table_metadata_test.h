#ifndef TEST_TABLE_METADATA_TEST_H_
#define TEST_TABLE_METADATA_TEST_H_

#include "table_metadata.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class TableMetadataTest : public testing::Test {
};

TEST_F(TableMetadataTest, ReadWriteTest) {
  metadata_writer<storage::durable> w("/tmp");
  w.write_index_info(0, "col1", 0.0);
  w.write_filter_info(1, "col1>0");
  w.write_trigger_info(2, 3, aggregate_id::D_SUM, "col1", relop_id::LT, mutable_value_t(3));

  metadata_reader r("/tmp");
  ASSERT_EQ(metadata_type::D_INDEX_METADATA, r.next_type());
  __index_info iinfo = r.next_index_info();
  ASSERT_EQ(0U, iinfo.index_id());
  ASSERT_EQ("col1", iinfo.name());
  ASSERT_EQ(static_cast<double>(0.0), iinfo.bucket_size());

  ASSERT_EQ(metadata_type::D_FILTER_METADATA, r.next_type());
  __filter_info finfo = r.next_filter_info();
  ASSERT_EQ(1U, finfo.filter_id());
  ASSERT_EQ("col1>0", finfo.expr());

  ASSERT_EQ(metadata_type::D_TRIGGER_METADATA, r.next_type());
  __trigger_info tinfo = r.next_trigger_info();
  ASSERT_EQ(2U, tinfo.trigger_id());
  ASSERT_EQ(3U, tinfo.filter_id());
  ASSERT_EQ(aggregate_id::D_SUM, tinfo.agg_id());
  ASSERT_EQ("col1", tinfo.name());
  ASSERT_EQ(relop_id::LT, tinfo.op());
  ASSERT_TRUE(mutable_value_t(3) == tinfo.threshold());

}

#endif /* TEST_TABLE_METADATA_TEST_H_ */

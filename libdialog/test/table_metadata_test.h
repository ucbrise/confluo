#ifndef TEST_TABLE_METADATA_TEST_H_
#define TEST_TABLE_METADATA_TEST_H_

#include "table_metadata.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class TableMetadataTest : public testing::Test {
};

TEST_F(TableMetadataTest, ReadWriteTest) {
  metadata_writer w("/tmp", storage::storage_id::D_DURABLE);
  w.write_index_info("col1", 0.0);
  w.write_filter_info("filter1", "col1>0");
  w.write_trigger_info("trigger1", "filter1", aggregate_id::D_SUM, "col1",
                       relop_id::LT, numeric(3));

  metadata_reader r("/tmp");
  ASSERT_EQ(metadata_type::D_INDEX_METADATA, r.next_type());
  index_info iinfo = r.next_index_info();
  ASSERT_EQ("col1", iinfo.field_name());
  ASSERT_EQ(static_cast<double>(0.0), iinfo.bucket_size());

  ASSERT_EQ(metadata_type::D_FILTER_METADATA, r.next_type());
  filter_info finfo = r.next_filter_info();
  ASSERT_EQ("filter1", finfo.filter_name());
  ASSERT_EQ("col1>0", finfo.expr());

  ASSERT_EQ(metadata_type::D_TRIGGER_METADATA, r.next_type());
  trigger_info tinfo = r.next_trigger_info();
  ASSERT_EQ("trigger1", tinfo.trigger_name());
  ASSERT_EQ("filter1", tinfo.filter_name());
  ASSERT_EQ(aggregate_id::D_SUM, tinfo.agg_id());
  ASSERT_EQ("col1", tinfo.field_name());
  ASSERT_EQ(relop_id::LT, tinfo.op());
  ASSERT_TRUE(numeric(3) == tinfo.threshold());
}

#endif /* TEST_TABLE_METADATA_TEST_H_ */

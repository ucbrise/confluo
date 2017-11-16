#ifndef TEST_TABLE_METADATA_TEST_H_
#define TEST_TABLE_METADATA_TEST_H_

#include "table_metadata.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class TableMetadataTest : public testing::Test {
};

TEST_F(TableMetadataTest, ReadWriteTest) {
  metadata_writer w("/tmp", storage::storage_id::D_DURABLE);
  schema_builder builder;
  builder.add_column(BOOL_TYPE, "a");
  builder.add_column(CHAR_TYPE, "b");
  builder.add_column(SHORT_TYPE, "c");
  builder.add_column(INT_TYPE, "d");
  builder.add_column(LONG_TYPE, "e");
  builder.add_column(FLOAT_TYPE, "f");
  builder.add_column(DOUBLE_TYPE, "g");
  builder.add_column(STRING_TYPE(16), "h");
  schema_t s(builder.get_columns());

  w.write_schema(s);
  w.write_index_info("col1", 0.0);
  w.write_filter_info("filter1", "col1>0");
  w.write_trigger_info("trigger1", "filter1", aggregate_id::D_SUM, "col1",
                       reational_op_id::LT, numeric(3), 10);

  metadata_reader r("/tmp");
  ASSERT_EQ(metadata_type::D_SCHEMA_METADATA, r.next_type());
  schema_t schema = r.next_schema();
  ASSERT_EQ(s.size(), schema.size());
  ASSERT_EQ(s.record_size(), schema.record_size());
  for (size_t i = 0; i < s.size(); i++) {
    ASSERT_EQ(s[i].name(), schema[i].name());
    ASSERT_EQ(s[i].type().id, schema[i].type().id);
    ASSERT_EQ(s[i].type().size, schema[i].type().size);
    if (s[i].type().id != primitive_type_id::D_STRING) {
      ASSERT_TRUE(s[i].min() == schema[i].min());
      ASSERT_TRUE(s[i].max() == schema[i].max());
    }
  }

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
  ASSERT_EQ(reational_op_id::LT, tinfo.op());
  ASSERT_TRUE(numeric(3) == tinfo.threshold());
  ASSERT_EQ(UINT64_C(10), tinfo.periodicity_ms());
}

#endif /* TEST_TABLE_METADATA_TEST_H_ */

#ifndef CONFLUO_TEST_TABLE_METADATA_TEST_H_
#define CONFLUO_TEST_TABLE_METADATA_TEST_H_

#include "gtest/gtest.h"
#include "atomic_multilog_metadata.h"

using namespace ::confluo;

class AtomicMultilogMetadataTest : public testing::Test {
};

TEST_F(AtomicMultilogMetadataTest, ReadWriteTest) {
  metadata_writer w("/tmp");
  schema_builder builder;
  builder.add_column(primitive_types::BOOL_TYPE(), "a");
  builder.add_column(primitive_types::CHAR_TYPE(), "b");
  builder.add_column(primitive_types::SHORT_TYPE(), "c");
  builder.add_column(primitive_types::INT_TYPE(), "d");
  builder.add_column(primitive_types::LONG_TYPE(), "e");
  builder.add_column(primitive_types::FLOAT_TYPE(), "f");
  builder.add_column(primitive_types::DOUBLE_TYPE(), "g");
  builder.add_column(primitive_types::STRING_TYPE(16), "h");
  schema_t s(builder.get_columns());

  w.write_schema(s);
  w.write_index_metadata("col1", 0.0);
  w.write_filter_metadata("filter1", "d>0");
  w.write_aggregate_metadata("agg1", "filter1", "SUM(d)");
  w.write_trigger_metadata("trigger1", "agg1<3", 10);

  metadata_reader r("/tmp");
  ASSERT_EQ(metadata_type::D_SCHEMA_METADATA, r.next_type());
  schema_t schema = r.next_schema();
  ASSERT_EQ(s.size(), schema.size());
  ASSERT_EQ(s.record_size(), schema.record_size());
  for (size_t i = 0; i < s.size(); i++) {
    ASSERT_EQ(s[i].name(), schema[i].name());
    ASSERT_EQ(s[i].type().id, schema[i].type().id);
    ASSERT_EQ(s[i].type().size, schema[i].type().size);
    if (s[i].type().id != primitive_type::D_STRING) {
      ASSERT_TRUE(s[i].min() == schema[i].min());
      ASSERT_TRUE(s[i].max() == schema[i].max());
    }
  }

  ASSERT_EQ(metadata_type::D_INDEX_METADATA, r.next_type());
  index_metadata iinfo = r.next_index_metadata();
  ASSERT_EQ("col1", iinfo.field_name());
  ASSERT_EQ(static_cast<double>(0.0), iinfo.bucket_size());

  ASSERT_EQ(metadata_type::D_FILTER_METADATA, r.next_type());
  filter_metadata finfo = r.next_filter_metadata();
  ASSERT_EQ("filter1", finfo.filter_name());
  ASSERT_EQ("d>0", finfo.expr());

  ASSERT_EQ(metadata_type::D_AGGREGATE_METADATA, r.next_type());
  aggregate_metadata ainfo = r.next_aggregate_metadata();
  ASSERT_EQ("agg1", ainfo.aggregate_name());
  ASSERT_EQ("filter1", ainfo.filter_name());
  ASSERT_EQ("SUM(d)", ainfo.aggregate_expression());

  ASSERT_EQ(metadata_type::D_TRIGGER_METADATA, r.next_type());
  trigger_metadata tinfo = r.next_trigger_metadata();
  ASSERT_EQ("trigger1", tinfo.trigger_name());
  ASSERT_EQ("agg1<3", tinfo.trigger_expression());
  ASSERT_EQ(UINT64_C(10), tinfo.periodicity_ms());
}

#endif /* CONFLUO_TEST_TABLE_METADATA_TEST_H_ */

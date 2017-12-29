#ifndef CONFLUO_TEST_RECORD_BATCH_TEST_H_
#define CONFLUO_TEST_RECORD_BATCH_TEST_H_

#include "schema/record_batch.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class RecordBatchTest : public testing::Test {
 public:
  struct rec {
    int64_t ts;
    bool a;
    int8_t b;
    int16_t c;
    int32_t d;
    int64_t e;
    float f;
    double g;
    char h[16];

    friend bool operator==(const rec& a, const rec& b) {
      return a.ts == b.ts && a.a == b.a && a.b == b.b && a.c == b.c
          && a.d == b.d && a.e == b.e && a.f == b.f && a.g == b.g;
    }
  }__attribute__((packed));

  static std::vector<column_t> s;
  static rec r;

  void* record(int64_t ts, bool a, int8_t b, int16_t c, int32_t d, int64_t e,
               float f, double g) {
    r = {ts, a, b, c, d, e, f, g, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0}};
    return &r;
  }

  std::string record_str(int64_t ts, bool a, int8_t b, int16_t c, int32_t d, int64_t e,
      float f, double g) {
    const char* str = reinterpret_cast<const char*>(record(ts, a, b, c, d, e, f, g));
    return std::string(str, sizeof(rec));
  }

  static std::vector<column_t> schema() {
    schema_builder builder;
    builder.add_column(BOOL_TYPE, "a");
    builder.add_column(CHAR_TYPE, "b");
    builder.add_column(SHORT_TYPE, "c");
    builder.add_column(INT_TYPE, "d");
    builder.add_column(LONG_TYPE, "e");
    builder.add_column(FLOAT_TYPE, "f");
    builder.add_column(DOUBLE_TYPE, "g");
    builder.add_column(STRING_TYPE(16), "h");
    return builder.get_columns();
  }
};

std::vector<column_t> RecordBatchTest::s = schema();
RecordBatchTest::rec RecordBatchTest::r;

TEST_F(RecordBatchTest, RecordBatchBuilderTest1) {
  schema_t schema(s);
  record_batch_builder builder(schema);
  typedef RecordBatchTest::rec rec;

  builder.add_record(record(0, true, 'a', 0, 0, 0, 0.0, 0.0));
  builder.add_record(record(0, false, 'b', 1, 1, 1, 1.0, 1.0));
  builder.add_record(record(0, true, 'c', 2, 2, 2, 2.0, 2.0));

  builder.add_record(record(1e6, false, 'd', 3, 3, 3, 3.0, 3.0));
  builder.add_record(record(1e6, true, 'e', 4, 4, 4, 4.0, 4.0));

  builder.add_record(record(2e6, false, 'f', 5, 5, 5, 5.0, 5.0));
  builder.add_record(record(2e6, true, 'g', 6, 6, 6, 6.0, 6.0));

  auto batch = builder.get_batch();

  ASSERT_EQ(static_cast<size_t>(3), batch.blocks.size());
  ASSERT_EQ(static_cast<size_t>(7), batch.nrecords);

  ASSERT_EQ(static_cast<int64_t>(0), batch.blocks[0].time_block);
  ASSERT_EQ(static_cast<size_t>(3), batch.blocks[0].nrecords);
  ASSERT_EQ(static_cast<size_t>(3) * sizeof(rec), batch.blocks[0].data.size());
  ASSERT_TRUE(
      record_str(0, true, 'a', 0, 0, 0, 0.0, 0.0)
          == batch.blocks[0].data.substr(0, sizeof(rec)));
  ASSERT_TRUE(
      record_str(0, false, 'b', 1, 1, 1, 1.0, 1.0)
          == batch.blocks[0].data.substr(sizeof(rec), sizeof(rec)));
  ASSERT_TRUE(
      record_str(0, true, 'c', 2, 2, 2, 2.0, 2.0)
          == batch.blocks[0].data.substr(2 * sizeof(rec), sizeof(rec)));

  ASSERT_EQ(static_cast<int64_t>(1), batch.blocks[1].time_block);
  ASSERT_EQ(static_cast<size_t>(2), batch.blocks[1].nrecords);
  ASSERT_EQ(static_cast<size_t>(2) * sizeof(rec), batch.blocks[1].data.size());
  ASSERT_TRUE(
      record_str(1e6, false, 'd', 3, 3, 3, 3.0, 3.0)
          == batch.blocks[1].data.substr(0, sizeof(rec)));
  ASSERT_TRUE(
      record_str(1e6, true, 'e', 4, 4, 4, 4.0, 4.0)
          == batch.blocks[1].data.substr(sizeof(rec), sizeof(rec)));

  ASSERT_EQ(static_cast<int64_t>(2), batch.blocks[2].time_block);
  ASSERT_EQ(static_cast<size_t>(2), batch.blocks[2].nrecords);
  ASSERT_EQ(static_cast<size_t>(2) * sizeof(rec), batch.blocks[2].data.size());
  ASSERT_TRUE(
      record_str(2e6, false, 'f', 5, 5, 5, 5.0, 5.0)
          == batch.blocks[2].data.substr(0, sizeof(rec)));
  ASSERT_TRUE(
      record_str(2e6, true, 'g', 6, 6, 6, 6.0, 6.0)
          == batch.blocks[2].data.substr(sizeof(rec), sizeof(rec)));
}

TEST_F(RecordBatchTest, RecordBatchBuilderTest2) {
  schema_t schema(s);
  record_batch_builder builder(schema);
  typedef RecordBatchTest::rec rec;

  builder.add_record( { "0", "true", "a", "0", "0", "0", "0.0", "0.0", "" });
  builder.add_record( { "0", "false", "b", "1", "1", "1", "1.0", "1.0", "" });
  builder.add_record( { "0", "true", "c", "2", "2", "2", "2.0", "2.0", "" });

  builder.add_record( { "1000000", "false", "d", "3", "3", "3", "3.0", "3.0", "" });
  builder.add_record( { "1000000", "true", "e", "4", "4", "4", "4.0", "4.0", "" });

  builder.add_record( { "2000000", "false", "f", "5", "5", "5", "5.0", "5.0", "" });
  builder.add_record( { "2000000", "true", "g", "6", "6", "6", "6.0", "6.0", "" });

  auto batch = builder.get_batch();

  ASSERT_EQ(static_cast<size_t>(3), batch.blocks.size());
  ASSERT_EQ(static_cast<size_t>(7), batch.nrecords);

  ASSERT_EQ(static_cast<int64_t>(0), batch.blocks[0].time_block);
  ASSERT_EQ(static_cast<size_t>(3), batch.blocks[0].nrecords);
  ASSERT_EQ(static_cast<size_t>(3) * sizeof(rec), batch.blocks[0].data.size());
  ASSERT_TRUE(
      record_str(0, true, 'a', 0, 0, 0, 0.0, 0.0)
          == batch.blocks[0].data.substr(0, sizeof(rec)));
  ASSERT_TRUE(
      record_str(0, false, 'b', 1, 1, 1, 1.0, 1.0)
          == batch.blocks[0].data.substr(sizeof(rec), sizeof(rec)));
  ASSERT_TRUE(
      record_str(0, true, 'c', 2, 2, 2, 2.0, 2.0)
          == batch.blocks[0].data.substr(2 * sizeof(rec), sizeof(rec)));

  ASSERT_EQ(static_cast<int64_t>(1), batch.blocks[1].time_block);
  ASSERT_EQ(static_cast<size_t>(2), batch.blocks[1].nrecords);
  ASSERT_EQ(static_cast<size_t>(2) * sizeof(rec), batch.blocks[1].data.size());
  ASSERT_TRUE(
      record_str(1e6, false, 'd', 3, 3, 3, 3.0, 3.0)
          == batch.blocks[1].data.substr(0, sizeof(rec)));
  ASSERT_TRUE(
      record_str(1e6, true, 'e', 4, 4, 4, 4.0, 4.0)
          == batch.blocks[1].data.substr(sizeof(rec), sizeof(rec)));

  ASSERT_EQ(static_cast<int64_t>(2), batch.blocks[2].time_block);
  ASSERT_EQ(static_cast<size_t>(2), batch.blocks[2].nrecords);
  ASSERT_EQ(static_cast<size_t>(2) * sizeof(rec), batch.blocks[2].data.size());
  ASSERT_TRUE(
      record_str(2e6, false, 'f', 5, 5, 5, 5.0, 5.0)
          == batch.blocks[2].data.substr(0, sizeof(rec)));
  ASSERT_TRUE(
      record_str(2e6, true, 'g', 6, 6, 6, 6.0, 6.0)
          == batch.blocks[2].data.substr(sizeof(rec), sizeof(rec)));
}

#endif /* CONFLUO_TEST_RECORD_BATCH_TEST_H_ */

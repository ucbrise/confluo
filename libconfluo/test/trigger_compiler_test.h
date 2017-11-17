#ifndef TEST_TRIGGER_COMPILER_TEST_H_
#define TEST_TRIGGER_COMPILER_TEST_H_

#include "parser/trigger_compiler.h"

#include "gtest/gtest.h"

using namespace ::confluo::parser;
using namespace ::confluo;

class TriggerCompilerTest : public testing::Test {
 public:
  static schema_t s;

  static schema_t schema() {
    schema_builder builder;
    builder.add_column(BOOL_TYPE, "a");
    builder.add_column(CHAR_TYPE, "b");
    builder.add_column(SHORT_TYPE, "c");
    builder.add_column(INT_TYPE, "d");
    builder.add_column(LONG_TYPE, "e");
    builder.add_column(FLOAT_TYPE, "f");
    builder.add_column(DOUBLE_TYPE, "g");
    builder.add_column(STRING_TYPE(16), "h");
    return schema_t(builder.get_columns());
  }
};

schema_t TriggerCompilerTest::s = schema();

TEST_F(TriggerCompilerTest, ParseTest) {
  auto t1 = compile_trigger(parse_trigger("SUM(c) < 10"), s);
  ASSERT_EQ(aggregate_id::D_SUM, t1.agg);
  ASSERT_EQ("c", t1.field_name);
  ASSERT_EQ(reational_op_id::LT, t1.relop);
  ASSERT_TRUE(numeric(static_cast<int16_t>(10)) == t1.threshold);

  auto t2 = compile_trigger(parse_trigger("MIN(d) >= 200"), s);
  ASSERT_EQ(aggregate_id::D_MIN, t2.agg);
  ASSERT_EQ("d", t2.field_name);
  ASSERT_EQ(reational_op_id::GE, t2.relop);
  ASSERT_TRUE(numeric(static_cast<int32_t>(200)) == t2.threshold);

  auto t3 = compile_trigger(parse_trigger("MAX(e) == 4000"), s);
  ASSERT_EQ(aggregate_id::D_MAX, t3.agg);
  ASSERT_EQ("e", t3.field_name);
  ASSERT_EQ(reational_op_id::EQ, t3.relop);
  ASSERT_TRUE(numeric(static_cast<int64_t>(4000)) == t3.threshold);

  auto t4 = compile_trigger(parse_trigger("CNT(d) <= 10"), s);
  ASSERT_EQ(aggregate_id::D_CNT, t4.agg);
  ASSERT_EQ("d", t4.field_name);
  ASSERT_EQ(reational_op_id::LE, t4.relop);
  ASSERT_TRUE(numeric(static_cast<int64_t>(10)) == t4.threshold);
}

#endif /* TEST_TRIGGER_COMPILER_TEST_H_ */

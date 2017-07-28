#ifndef TEST_TRIGGER_PARSER_TEST_H_
#define TEST_TRIGGER_PARSER_TEST_H_

#include "trigger_parser.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class TriggerParserTest : public testing::Test {
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
    return schema_t(".", builder.get_columns());
  }
};

schema_t TriggerParserTest::s = schema();

TEST_F(TriggerParserTest, ParseTest) {
  trigger_parser parser1("SUM(c) < 10", s);
  auto t1 = parser1.parse();
  ASSERT_EQ(aggregate_id::D_SUM, t1.agg);
  ASSERT_EQ("c", t1.field_name);
  ASSERT_EQ(relop_id::LT, t1.relop);
  ASSERT_TRUE(numeric(static_cast<int16_t>(10)) == t1.threshold);

  trigger_parser parser2("MIN(d) >= 200", s);
  auto t2 = parser2.parse();
  ASSERT_EQ(aggregate_id::D_MIN, t2.agg);
  ASSERT_EQ("d", t2.field_name);
  ASSERT_EQ(relop_id::GE, t2.relop);
  ASSERT_TRUE(numeric(static_cast<int32_t>(200)) == t2.threshold);

  trigger_parser parser3("MAX(e) == 4000", s);
  auto t3 = parser3.parse();
  ASSERT_EQ(aggregate_id::D_MAX, t3.agg);
  ASSERT_EQ("e", t3.field_name);
  ASSERT_EQ(relop_id::EQ, t3.relop);
  ASSERT_TRUE(numeric(static_cast<int64_t>(4000)) == t3.threshold);

  trigger_parser parser4("CNT(d) <= 10", s);
  auto t4 = parser4.parse();
  ASSERT_EQ(aggregate_id::D_CNT, t4.agg);
  ASSERT_EQ("d", t4.field_name);
  ASSERT_EQ(relop_id::LE, t4.relop);
  ASSERT_TRUE(numeric(static_cast<int64_t>(10)) == t4.threshold);
}

#endif /* TEST_TRIGGER_PARSER_TEST_H_ */

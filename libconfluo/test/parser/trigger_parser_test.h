#ifndef CONFLUO_TEST_TRIGGER_PARSER_TEST_H_
#define CONFLUO_TEST_TRIGGER_PARSER_TEST_H_

#include "parser/trigger_parser.h"

#include "gtest/gtest.h"

using namespace ::confluo::parser;

class TriggerParserTest : public testing::Test {
};

TEST_F(TriggerParserTest, ParseTest) {
  auto t1 = parse_trigger("SUM(c) < 10");
  ASSERT_EQ("SUM", t1.agg);
  ASSERT_EQ("c", t1.field_name);
  ASSERT_EQ("<", t1.relop);
  ASSERT_EQ("10", t1.threshold);

  auto t2 = parse_trigger("MIN(d) >= 200");
  ASSERT_EQ("MIN", t2.agg);
  ASSERT_EQ("d", t2.field_name);
  ASSERT_EQ(">=", t2.relop);
  ASSERT_EQ("200", t2.threshold);

  auto t3 = parse_trigger("MAX(e) == 4000");
  ASSERT_EQ("MAX", t3.agg);
  ASSERT_EQ("e", t3.field_name);
  ASSERT_EQ("==", t3.relop);
  ASSERT_EQ("4000", t3.threshold);

  auto t4 = parse_trigger("CNT(d) <= 10");
  ASSERT_EQ("CNT", t4.agg);
  ASSERT_EQ("d", t4.field_name);
  ASSERT_EQ("<=", t4.relop);
  ASSERT_EQ("10", t4.threshold);
}

#endif /* CONFLUO_TEST_TRIGGER_PARSER_TEST_H_ */

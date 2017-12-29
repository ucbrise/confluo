#ifndef CONFLUO_TEST_TRIGGER_PARSER_TEST_H_
#define CONFLUO_TEST_TRIGGER_PARSER_TEST_H_

#include "parser/trigger_parser.h"

#include "gtest/gtest.h"

using namespace ::confluo::parser;

class TriggerParserTest : public testing::Test {
};

TEST_F(TriggerParserTest, ParseTest) {
  auto t1 = parse_trigger("SUM_c < 10");
  ASSERT_EQ("SUM_c", t1.aggregate_name);
  ASSERT_EQ("<", t1.relop);
  ASSERT_EQ("10", t1.threshold);

  auto t2 = parse_trigger("MIN_d >= 200");
  ASSERT_EQ("MIN_d", t2.aggregate_name);
  ASSERT_EQ(">=", t2.relop);
  ASSERT_EQ("200", t2.threshold);

  auto t3 = parse_trigger("MAX_e == 4000");
  ASSERT_EQ("MAX_e", t3.aggregate_name);
  ASSERT_EQ("==", t3.relop);
  ASSERT_EQ("4000", t3.threshold);

  auto t4 = parse_trigger("CNT_d <= 10");
  ASSERT_EQ("CNT_d", t4.aggregate_name);
  ASSERT_EQ("<=", t4.relop);
  ASSERT_EQ("10", t4.threshold);
}

#endif /* CONFLUO_TEST_TRIGGER_PARSER_TEST_H_ */

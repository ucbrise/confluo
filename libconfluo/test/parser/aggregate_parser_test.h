#ifndef CONFLUO_TEST_AGGREGATE_PARSER_TEST_H_
#define CONFLUO_TEST_AGGREGATE_PARSER_TEST_H_

#include "parser/aggregate_parser.h"

#include "gtest/gtest.h"

using namespace ::confluo::parser;

class AggregateParserTest : public testing::Test {
};

TEST_F(AggregateParserTest, ParseTest) {
  auto t1 = parse_aggregate("SUM(c)");
  ASSERT_EQ("SUM", t1.agg);
  ASSERT_EQ("c", t1.field_name);

  auto t2 = parse_aggregate("MIN(d)");
  ASSERT_EQ("MIN", t2.agg);
  ASSERT_EQ("d", t2.field_name);

  auto t3 = parse_aggregate("MAX(e)");
  ASSERT_EQ("MAX", t3.agg);
  ASSERT_EQ("e", t3.field_name);

  auto t4 = parse_aggregate("CNT(d)");
  ASSERT_EQ("CNT", t4.agg);
  ASSERT_EQ("d", t4.field_name);
}

#endif /* CONFLUO_TEST_AGGREGATE_PARSER_TEST_H_ */

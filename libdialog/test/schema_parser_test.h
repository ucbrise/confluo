#ifndef TEST_SCHEMA_PARSER_TEST_H_
#define TEST_SCHEMA_PARSER_TEST_H_

#include <sstream>

#include "gtest/gtest.h"
#include "parser/schema_parser.h"

using namespace ::confluo::parser;

class SchemaParserTest : public testing::Test {
};

TEST_F(SchemaParserTest, BasicTest) {
  std::string schema = "{a: BOOL, b: CHAR, c: SHORT, d: INT, e: LONG, f: FLOAT,"
      " g: DOUBLE, h: STRING(16)}";
  auto schema_vec = parse_schema(schema);
  ASSERT_EQ(static_cast<size_t>(9), schema_vec.size());

    ASSERT_EQ(0, schema_vec[0].idx());
    ASSERT_EQ(0, schema_vec[0].offset());
    ASSERT_EQ("TIMESTAMP", schema_vec[0].name());
    ASSERT_TRUE(LONG_TYPE == schema_vec[0].type());
    ASSERT_FALSE(schema_vec[0].is_indexed());

    ASSERT_EQ(1, schema_vec[1].idx());
    ASSERT_EQ(8, schema_vec[1].offset());
    ASSERT_EQ("A", schema_vec[1].name());
    ASSERT_TRUE(BOOL_TYPE == schema_vec[1].type());
    ASSERT_FALSE(schema_vec[1].is_indexed());

    ASSERT_EQ(2, schema_vec[2].idx());
    ASSERT_EQ(9, schema_vec[2].offset());
    ASSERT_EQ("B", schema_vec[2].name());
    ASSERT_TRUE(CHAR_TYPE == schema_vec[2].type());
    ASSERT_FALSE(schema_vec[2].is_indexed());

    ASSERT_EQ(3, schema_vec[3].idx());
    ASSERT_EQ(10, schema_vec[3].offset());
    ASSERT_EQ("C", schema_vec[3].name());
    ASSERT_TRUE(SHORT_TYPE == schema_vec[3].type());
    ASSERT_FALSE(schema_vec[3].is_indexed());

    ASSERT_EQ(4, schema_vec[4].idx());
    ASSERT_EQ(12, schema_vec[4].offset());
    ASSERT_EQ("D", schema_vec[4].name());
    ASSERT_TRUE(INT_TYPE == schema_vec[4].type());
    ASSERT_FALSE(schema_vec[4].is_indexed());

    ASSERT_EQ(5, schema_vec[5].idx());
    ASSERT_EQ(16, schema_vec[5].offset());
    ASSERT_EQ("E", schema_vec[5].name());
    ASSERT_TRUE(LONG_TYPE == schema_vec[5].type());
    ASSERT_FALSE(schema_vec[5].is_indexed());

    ASSERT_EQ(6, schema_vec[6].idx());
    ASSERT_EQ(24, schema_vec[6].offset());
    ASSERT_EQ("F", schema_vec[6].name());
    ASSERT_TRUE(FLOAT_TYPE == schema_vec[6].type());
    ASSERT_FALSE(schema_vec[6].is_indexed());

    ASSERT_EQ(7, schema_vec[7].idx());
    ASSERT_EQ(28, schema_vec[7].offset());
    ASSERT_EQ("G", schema_vec[7].name());
    ASSERT_TRUE(DOUBLE_TYPE == schema_vec[7].type());
    ASSERT_FALSE(schema_vec[7].is_indexed());

    ASSERT_EQ(8, schema_vec[8].idx());
    ASSERT_EQ(36, schema_vec[8].offset());
    ASSERT_EQ("H", schema_vec[8].name());
    ASSERT_TRUE(STRING_TYPE(16) == schema_vec[8].type());
    ASSERT_FALSE(schema_vec[8].is_indexed());
}

#endif /* TEST_SCHEMA_PARSER_TEST_H_ */

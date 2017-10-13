#ifndef TEST_EXPRESSION_COMPILER_TEST_H_
#define TEST_EXPRESSION_COMPILER_TEST_H_

#include "parser/expression_compiler.h"
#include "schema.h"
#include "gtest/gtest.h"
#include "parser/expression_parser.h"

using namespace ::dialog::parser;
using namespace ::dialog;

class ExpressionCompilerTest : public testing::Test {
 public:
  static void compile(compiled_expression& cexp, const std::string& exp,
                      const schema_t& schema) {
    auto t = parse_expression(exp);
    cexp = compile_expression(t, schema);
  }

  static void check_predicate(const compiled_predicate& c) {
    if (c.to_string() == "A==bool(1)") {
      ASSERT_EQ("A", c.field_name());
      ASSERT_TRUE(BOOL_TYPE == c.field_type());
      ASSERT_EQ(relop_id::EQ, c.op());
    } else if (c.to_string() == "B<char(53)") {
      ASSERT_EQ("B", c.field_name());
      ASSERT_TRUE(CHAR_TYPE == c.field_type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "C<short(10)") {
      ASSERT_EQ("C", c.field_name());
      ASSERT_TRUE(SHORT_TYPE == c.field_type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "E<long(10)") {
      ASSERT_EQ("E", c.field_name());
      ASSERT_TRUE(LONG_TYPE == c.field_type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "F<float(1.300000)") {
      ASSERT_EQ("F", c.field_name());
      ASSERT_TRUE(FLOAT_TYPE == c.field_type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "G<double(1.900000)") {
      ASSERT_EQ("G", c.field_name());
      ASSERT_TRUE(DOUBLE_TYPE == c.field_type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else {
      ASSERT_TRUE(false);
    }
  }
};

TEST_F(ExpressionCompilerTest, CompilerTest) {
  schema_builder builder;
  builder.add_column(BOOL_TYPE, "a");
  builder.add_column(CHAR_TYPE, "b");
  builder.add_column(SHORT_TYPE, "c");
  builder.add_column(INT_TYPE, "d");
  builder.add_column(LONG_TYPE, "e");
  builder.add_column(FLOAT_TYPE, "f");
  builder.add_column(DOUBLE_TYPE, "g");
  builder.add_column(STRING_TYPE(16), "h");
  builder.add_column(STRING_TYPE(10), "h");

  schema_t s(builder.get_columns());

  compiled_expression m1, m2, m3, m4, m5, m6, m7;
  compile(m1, "a==true", s);
  ASSERT_EQ(static_cast<size_t>(1), m1.size());
  for (auto& m : m1) {
    ASSERT_EQ(static_cast<size_t>(1), m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }

  compile(m2, "a==true && b<5", s);
  ASSERT_EQ(static_cast<size_t>(1), m2.size());
  for (auto& m : m2) {
    ASSERT_EQ(static_cast<size_t>(2), m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }

  compile(m3, "a==true || b<5", s);
  ASSERT_EQ(static_cast<size_t>(2), m3.size());
  for (auto& m : m3) {
    ASSERT_EQ(static_cast<size_t>(1), m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }

  compile(m4, "a==true && b<5 || c<10", s);
  ASSERT_EQ(static_cast<size_t>(2), m4.size());
  for (auto& m : m4) {
    ASSERT_TRUE(
        static_cast<size_t>(2) == m.size()
            || static_cast<size_t>(1) == m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }

  compile(m5, "a==true && (b<5 || c<10)", s);
  ASSERT_EQ(static_cast<size_t>(2), m5.size());
  for (auto& m : m5) {
    ASSERT_EQ(static_cast<size_t>(2), m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }

  compile(m6, "a==true && (b<5 || (c<10 && e<10))", s);
  ASSERT_EQ(static_cast<size_t>(2), m6.size());
  for (auto& m : m6) {
    ASSERT_TRUE(
        static_cast<size_t>(2) == m.size()
            || static_cast<size_t>(3) == m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }

  compile(m7, "a==true && (b<5 || c<10 || e<10 || f<1.3 || g<1.9)", s);
  ASSERT_EQ(static_cast<size_t>(5), m7.size());
  for (auto& m : m7) {
    ASSERT_EQ(static_cast<size_t>(2), m.size());
    for (auto& c : m) {
      check_predicate(c);
    }
  }
}

#endif // TEST_EXPRESSION_COMPILER_TEST_H_

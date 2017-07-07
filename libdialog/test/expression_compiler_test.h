#ifndef TEST_EXPRESSION_COMPILER_TEST_H_
#define TEST_EXPRESSION_COMPILER_TEST_H_

#include "expression_compiler.h"

#include "schema.h"
#include "storage.h"

#include "gtest/gtest.h"

using namespace ::dialog::storage;
using namespace ::dialog;

class ExpressionCompilerTest : public testing::Test {
 public:
  static expression_t* build_expression(const std::string& exp) {
    parser p(exp);
    return p.parse();
  }

  static void print_expression(expression_t* exp) {
    expression_utils::print_expression(exp);
    fprintf(stderr, "\n");
  }

  static void compile(compiled_expression& cexp, const std::string& exp,
                      const schema_t<storage::in_memory>& schema) {
    expression_compiler::compile(cexp, exp, schema);
  }

  static void print_minterms(const compiled_expression& cexp) {
    for (const minterm& mterm : cexp) {
      fprintf(stderr, "{");
      for (auto& predicate : mterm)
        fprintf(stderr, "[%s]", predicate.to_string().c_str());
      fprintf(stderr, "}");
    }
    fprintf(stderr, "\n");
  }

  static void check_predicate(const compiled_predicate& c) {
    if (c.to_string() == "a==true") {
      ASSERT_EQ("A", c.column().name());
      ASSERT_TRUE(BOOL_TYPE == c.column().type());
      ASSERT_EQ(relop_id::EQ, c.op());
    } else if (c.to_string() == "b<5") {
      ASSERT_EQ("B", c.column().name());
      ASSERT_TRUE(CHAR_TYPE == c.column().type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "c<10") {
      ASSERT_EQ("C", c.column().name());
      ASSERT_TRUE(SHORT_TYPE == c.column().type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "e<10") {
      ASSERT_EQ("E", c.column().name());
      ASSERT_TRUE(LONG_TYPE == c.column().type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "f<1.3") {
      ASSERT_EQ("F", c.column().name());
      ASSERT_TRUE(FLOAT_TYPE == c.column().type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else if (c.to_string() == "g<1.9") {
      ASSERT_EQ("G", c.column().name());
      ASSERT_TRUE(DOUBLE_TYPE == c.column().type());
      ASSERT_EQ(relop_id::LT, c.op());
    } else {
      ASSERT_TRUE(false);
    }
  }
};

TEST_F(ExpressionCompilerTest, CompilerTest) {
  schema_builder builder;
  builder.add_column(bool_type(), "a");
  builder.add_column(char_type(), "b");
  builder.add_column(short_type(), "c");
  builder.add_column(int_type(), "d");
  builder.add_column(long_type(), "e");
  builder.add_column(float_type(), "f");
  builder.add_column(double_type(), "g");
  builder.add_column(string_type(10), "h");

  schema_t<in_memory> s(".", builder.get_schema());

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

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
  print_minterms(m1);

  compile(m2, "a==true && b<5", s);
  print_minterms(m2);

  compile(m3, "a==true || b<5", s);
  print_minterms(m3);

  compile(m4, "a==true && b<5 || c<10", s);
  print_minterms(m4);

  compile(m5, "a==true && (b<5 || c<10)", s);
  print_minterms(m5);

  compile(m6, "a==true && (b<5 || (c<10 && e<10))", s);
  print_minterms(m6);

  compile(m7, "a==true && (b<5 || c<10 || e<10 || f<1.3 || g<1.9)", s);
  print_minterms(m7);
}

#endif // TEST_EXPRESSION_COMPILER_TEST_H_

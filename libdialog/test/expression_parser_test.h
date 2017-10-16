#ifndef TEST_SCHEMA_TOKENIZER_TEST_H_
#define TEST_SCHEMA_TOKENIZER_TEST_H_

#include "parser/expression_parser.h"

#include <sstream>
#include <vector>

#include "gtest/gtest.h"

using namespace ::dialog::parser;

class ExpressionParserTest : public testing::Test {
 public:
  static void test_predicate(const spirit::utree& t, const std::string& op1,
                             int op, const std::string& op2) {
    auto it = t.begin();
    ASSERT_EQ(op, spirit::utree::visit(*it, utree_to_op()));
    ASSERT_EQ(op1, spirit::utree::visit(*(++it), utree_to_string()));
    ASSERT_EQ(op2, spirit::utree::visit(*(++it), utree_to_string()));
  }

  static void test_or(const spirit::utree& t) {
    auto it = t.begin();
    ASSERT_EQ(and_or::OR, spirit::utree::visit(*it, utree_to_op()));
  }

  static void test_and(const spirit::utree& t) {
    auto it = t.begin();
    ASSERT_EQ(and_or::AND, spirit::utree::visit(*it, utree_to_op()));
  }

  static spirit::utree left(const spirit::utree& t) {
    auto it = t.begin();
    return *(++it);
  }

  static spirit::utree right(const spirit::utree& t) {
    auto it = t.begin();
    ++it;
    return *(++it);
  }
};

TEST_F(ExpressionParserTest, ParsePredicateTest) {
  auto t1 = parse_expression("a < b");
  test_predicate(t1, "a", relop_id::LT, "b");

  auto t2 = parse_expression("a1 > b1");
  test_predicate(t2, "a1", relop_id::GT, "b1");

  auto t3 = parse_expression("abc <= def");
  test_predicate(t3, "abc", relop_id::LE, "def");

  auto t4 = parse_expression("a_1 == b_1");
  test_predicate(t4, "a_1", relop_id::EQ, "b_1");
}

TEST_F(ExpressionParserTest, ParseOrTest) {
  auto t1 = parse_expression("a < b || b > c");
  test_or(t1);
  test_predicate(left(t1), "a", relop_id::LT, "b");
  test_predicate(right(t1), "b", relop_id::GT, "c");

  auto t2 = parse_expression("a < b || b > c || c == d");
  test_or(t2);
  test_or(left(t2));
  test_predicate(left(left(t2)), "a", relop_id::LT, "b");
  test_predicate(right(left(t2)), "b", relop_id::GT, "c");
  test_predicate(right(t2), "c", relop_id::EQ, "d");

  auto t3 = parse_expression("a < b || (b > c || c == d)");
  test_or(t3);
  test_predicate(left(t3), "a", relop_id::LT, "b");
  test_or(right(t3));
  test_predicate(left(right(t3)), "b", relop_id::GT, "c");
  test_predicate(right(right(t3)), "c", relop_id::EQ, "d");
}

TEST_F(ExpressionParserTest, ParseAndTest) {
  auto t1 = parse_expression("a < b && b > c");
  test_and(t1);
  test_predicate(left(t1), "a", relop_id::LT, "b");
  test_predicate(right(t1), "b", relop_id::GT, "c");

  auto t2 = parse_expression("a < b && b > c && c == d");
  test_and(t2);
  test_and(left(t2));
  test_predicate(left(left(t2)), "a", relop_id::LT, "b");
  test_predicate(right(left(t2)), "b", relop_id::GT, "c");
  test_predicate(right(t2), "c", relop_id::EQ, "d");

  auto t3 = parse_expression("a < b && (b > c && c == d)");
  test_and(t3);
  test_predicate(left(t3), "a", relop_id::LT, "b");
  test_and(right(t3));
  test_predicate(left(right(t3)), "b", relop_id::GT, "c");
  test_predicate(right(right(t3)), "c", relop_id::EQ, "d");
}

TEST_F(ExpressionParserTest, ParseNotTest) {
  auto t1 = parse_expression("!(a < b)");
  test_predicate(t1, "a", relop_id::GE, "b");

  auto t2 = parse_expression("!(a < b && b > c)");
  test_or(t2);
  test_predicate(left(t2), "a", relop_id::GE, "b");
  test_predicate(right(t2), "b", relop_id::LE, "c");

  auto t3 = parse_expression("!(a < b || b > c)");
  test_and(t3);
  test_predicate(left(t3), "a", relop_id::GE, "b");
  test_predicate(right(t3), "b", relop_id::LE, "c");

  auto t4 = parse_expression("!!(a < b)");
  test_predicate(t4, "a", relop_id::LT, "b");
}

TEST_F(ExpressionParserTest, ParseAndOrTest) {
  auto exp1 = parse_expression("a < b && b > c || c == d");
  test_or(exp1);
  test_and(left(exp1));
  test_predicate(left(left(exp1)), "a", relop_id::LT, "b");
  test_predicate(right(left(exp1)), "b", relop_id::GT, "c");
  test_predicate(right(exp1), "c", relop_id::EQ, "d");

  auto exp2 = parse_expression("a < b && (b > c || c == d)");
  test_and(exp2);
  test_predicate(left(exp2), "a", relop_id::LT, "b");
  test_or(right(exp2));
  test_predicate(left(right(exp2)), "b", relop_id::GT, "c");
  test_predicate(right(right(exp2)), "c", relop_id::EQ, "d");
}

#endif /* TEST_SCHEMA_TOKENIZER_TEST_H_ */

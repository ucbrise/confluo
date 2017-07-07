#ifndef TEST_EXPRESSIONS_TEST_H_
#define TEST_EXPRESSIONS_TEST_H_

#include "expression.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class ExpressionTest : public testing::Test {
 public:
  static expression_t* build_expression(const std::string& exp) {
    parser p(exp);
    return p.parse();
  }

  static void print_expression(expression_t* exp) {
    expression_utils::print_expression(exp);
    fprintf(stderr, "\n");
  }

  static void test_predicate(expression_t* exp, const std::string& op1,
                             relop_id opid, const std::string& op2) {

    ASSERT_EQ(expression_id::PREDICATE, exp->id);
    predicate_t* p = (predicate_t*) exp;
    ASSERT_EQ(op1, p->attr);
    ASSERT_EQ(op2, p->value);
    ASSERT_EQ(opid, p->op);
  }

  static void test_or(expression_t* exp) {
    ASSERT_EQ(expression_id::OR, exp->id);
  }

  static void test_and(expression_t* exp) {
    ASSERT_EQ(expression_id::AND, exp->id);
  }
};

TEST_F(ExpressionTest, ParsePredicateTest) {
  expression_t* exp1 = build_expression("a < b");
  test_predicate(exp1, "a", relop_id::LT, "b");

  expression_t* exp2 = build_expression("a1 > b1");
  test_predicate(exp2, "a1", relop_id::GT, "b1");

  expression_t* exp3 = build_expression("abc <= def");
  test_predicate(exp3, "abc", relop_id::LE, "def");

  expression_t* exp4 = build_expression("1a >= 1b");
  test_predicate(exp4, "1a", relop_id::GE, "1b");

  expression_t* exp5 = build_expression("a_1 == b_1");
  test_predicate(exp5, "a_1", relop_id::EQ, "b_1");

  expression_t* exp6 = build_expression("a.1 != b.1");
  test_predicate(exp6, "a.1", relop_id::NEQ, "b.1");
}

TEST_F(ExpressionTest, ParseOrTest) {
  expression_t* exp1 = build_expression("a < b || b > c");
  test_or(exp1);
  disjunction_t *d1 = (disjunction_t*) exp1;
  test_predicate(d1->left, "a", relop_id::LT, "b");
  test_predicate(d1->right, "b", relop_id::GT, "c");

  expression_t* exp2 = build_expression("a < b || b > c || c == d");
  test_or(exp2);
  disjunction_t *d2 = (disjunction_t*) exp2;
  test_predicate(d2->left, "a", relop_id::LT, "b");
  test_or(d2->right);
  disjunction_t *d3 = (disjunction_t*) d2->right;
  test_predicate(d3->left, "b", relop_id::GT, "c");
  test_predicate(d3->right, "c", relop_id::EQ, "d");

  expression_t* exp3 = build_expression("(a < b || b > c) || c == d");
  test_or(exp3);
  disjunction_t *d4 = (disjunction_t*) exp3;
  test_or(d4->left);
  disjunction_t *d5 = (disjunction_t*) d4->left;
  test_predicate(d5->left, "a", relop_id::LT, "b");
  test_predicate(d5->right, "b", relop_id::GT, "c");
  test_predicate(d4->right, "c", relop_id::EQ, "d");
}

TEST_F(ExpressionTest, ParseAndTest) {
  expression_t* exp1 = build_expression("a < b && b > c");
  test_and(exp1);
  conjunction_t *c1 = (conjunction_t*) exp1;
  test_predicate(c1->left, "a", relop_id::LT, "b");
  test_predicate(c1->right, "b", relop_id::GT, "c");

  expression_t* exp2 = build_expression("a < b && b > c && c == d");
  test_and(exp2);
  conjunction_t *c2 = (conjunction_t*) exp2;
  test_predicate(c2->left, "a", relop_id::LT, "b");
  test_and(c2->right);
  conjunction_t *c3 = (conjunction_t*) c2->right;
  test_predicate(c3->left, "b", relop_id::GT, "c");
  test_predicate(c3->right, "c", relop_id::EQ, "d");

  expression_t* exp3 = build_expression("(a < b && b > c) && c == d");
  test_and(exp3);
  conjunction_t *c4 = (conjunction_t*) exp3;
  test_and(c4->left);
  conjunction_t *c5 = (conjunction_t*) c4->left;
  test_predicate(c5->left, "a", relop_id::LT, "b");
  test_predicate(c5->right, "b", relop_id::GT, "c");
  test_predicate(c4->right, "c", relop_id::EQ, "d");
}

TEST_F(ExpressionTest, ParseNotTest) {
  expression_t* exp1 = build_expression("!(a < b)");
  test_predicate(exp1, "a", relop_id::GE, "b");

  expression_t* exp2 = build_expression("!(a < b && b > c)");
  test_or(exp2);
  disjunction_t *d1 = (disjunction_t*) exp2;
  test_predicate(d1->left, "a", relop_id::GE, "b");
  test_predicate(d1->right, "b", relop_id::LE, "c");

  expression_t* exp3 = build_expression("!(a < b || b > c)");
  test_and(exp3);
  conjunction_t *c1 = (conjunction_t*) exp3;
  test_predicate(c1->left, "a", relop_id::GE, "b");
  test_predicate(c1->right, "b", relop_id::LE, "c");

  expression_t* exp4 = build_expression("!!(a < b)");
  test_predicate(exp4, "a", relop_id::LT, "b");
}

TEST_F(ExpressionTest, ParseAndOrTest) {
  expression_t* exp1 = build_expression("a < b && b > c || c == d");
  test_or(exp1);
  disjunction_t *d1 = (disjunction_t*) exp1;
  test_and(d1->left);
  test_predicate(d1->right, "c", relop_id::EQ, "d");
  conjunction_t *c1 = (conjunction_t*) d1->left;
  test_predicate(c1->left, "a", relop_id::LT, "b");
  test_predicate(c1->right, "b", relop_id::GT, "c");

  expression_t* exp2 = build_expression("a < b && (b > c || c == d)");
  test_and(exp2);
  conjunction_t *c2 = (conjunction_t*) exp2;
  test_predicate(c2->left, "a", relop_id::LT, "b");
  test_or(c2->right);
  disjunction_t *d2 = (disjunction_t*) c2->right;
  test_predicate(d2->left, "b", relop_id::GT, "c");
  test_predicate(d2->right, "c", relop_id::EQ, "d");
}

#endif // TEST_EXPRESSIONS_TEST_H_

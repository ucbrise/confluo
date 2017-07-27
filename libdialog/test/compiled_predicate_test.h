#ifndef TEST_COMPILED_PREDICATE_TEST_H_
#define TEST_COMPILED_PREDICATE_TEST_H_

#include "schema.h"
#include "storage.h"
#include "compiled_predicate.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class CompiledPredicateTest : public testing::Test {
 public:
  static schema_t s;

  struct rec {
    uint64_t ts;
    bool a;
    char b;
    short c;
    int d;
    long e;
    float f;
    double g;
    char h[16];
  }__attribute__((packed));

  static rec r;

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

  record_t record(bool a, char b, short c, int d, long e, float f, double g) {
    r = {UINT64_C(0), a, b, c, d, e, f, g, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0}};
    return s.apply(0, &r);
  }

  static compiled_predicate predicate(const std::string& attr, relop_id id,
      const std::string& value) {
    predicate_t p;
    p.attr = attr;
    p.op = id;
    p.value = value;
    compiled_predicate c(p, s);
    return c;
  }
};

CompiledPredicateTest::rec CompiledPredicateTest::r;

schema_t CompiledPredicateTest::s = CompiledPredicateTest::schema();

TEST_F(CompiledPredicateTest, TestPredicateTest) {
  ASSERT_TRUE(
      predicate("a", relop_id::EQ, "true").test(
          record(true, 0, 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("b", relop_id::LT, "c").test(
          record(false, 'a', 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("c", relop_id::LE, "10").test(
          record(false, 0, 10, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("d", relop_id::GT, "100").test(
          record(false, 0, 0, 101, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("e", relop_id::GE, "1000").test(
          record(false, 0, 0, 0, 1000, 0, 0)));

  ASSERT_TRUE(
      predicate("f", relop_id::NEQ, "100.3").test(
          record(false, 0, 0, 0, 0, 102.4, 0)));

  ASSERT_TRUE(
      predicate("g", relop_id::LT, "194.312").test(
          record(false, 0, 0, 0, 0, 0, 182.3)));
}

#endif /* TEST_COMPILED_PREDICATE_TEST_H_ */

#ifndef TEST_COMPILED_PREDICATE_TEST_H_
#define TEST_COMPILED_PREDICATE_TEST_H_

#include "schema.h"
#include "storage.h"
#include "compiled_predicate.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class CompiledPredicateTest : public testing::Test {
 public:
  static schema_t<storage::in_memory> s;

  struct rec {
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

  static schema_t<storage::in_memory> schema() {
    schema_builder builder;
    builder.add_column(bool_type(), "a");
    builder.add_column(char_type(), "b");
    builder.add_column(short_type(), "c");
    builder.add_column(int_type(), "d");
    builder.add_column(long_type(), "e");
    builder.add_column(float_type(), "f");
    builder.add_column(double_type(), "g");
    builder.add_column(string_type(16), "h");
    return schema_t<storage::in_memory>(".", builder.get_columns());
  }

  record_t record(bool a, char b, short c, int d, long e, float f, double g) {
    r = {a, b, c, d, e, f, g, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0}};
    return s.apply(0, &r, sizeof(rec), 0);
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

schema_t<storage::in_memory> CompiledPredicateTest::s =
    CompiledPredicateTest::schema();

TEST_F(CompiledPredicateTest, TestPredicateTest) {
  ASSERT_TRUE(
      predicate("a", relop_id::EQ, "true").test(
          record(true, 0, 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("b", relop_id::LT, "c").test(record(true, 'a', 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("c", relop_id::LE, "10").test(record(true, 0, 10, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("d", relop_id::GT, "100").test(
          record(true, 0, 0, 101, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("e", relop_id::GE, "1000").test(
          record(true, 0, 0, 0, 1000, 0, 0)));

  ASSERT_TRUE(
      predicate("f", relop_id::NEQ, "100.3").test(
          record(true, 0, 0, 0, 0, 102.4, 0)));

  ASSERT_TRUE(
      predicate("g", relop_id::LT, "194.312").test(
          record(true, 0, 0, 0, 0, 0, 182.3)));
}

#endif /* TEST_COMPILED_PREDICATE_TEST_H_ */

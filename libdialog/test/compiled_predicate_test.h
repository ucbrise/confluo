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
    int64_t ts;
    bool a;
    int8_t b;
    int16_t c;
    int32_t d;
    int64_t e;
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
    return schema_t(builder.get_columns());
  }

  void* record_buf(bool a, int8_t b, int16_t c, int32_t d, int64_t e, float f,
                   double g) {
    r = {INT64_C(0), a, b, c, d, e, f, g, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0}};
    return &r;
  }

  record_t record(bool a, int8_t b, int16_t c, int32_t d, int64_t e, float f,
      double g) {
    r = {INT64_C(0), a, b, c, d, e, f, g, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0}};
    return s.apply(0, &r);
  }

  static compiled_predicate predicate(const std::string& attr, relop_id id,
      const std::string& value) {
    return compiled_predicate(attr, id, value, s);
  }
};

CompiledPredicateTest::rec CompiledPredicateTest::r;

schema_t CompiledPredicateTest::s = CompiledPredicateTest::schema();

TEST_F(CompiledPredicateTest, TestPredicateRecordTest) {
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

TEST_F(CompiledPredicateTest, TestPredicateBufferTest) {
  auto snap = s.snapshot();
  ASSERT_TRUE(
      predicate("a", relop_id::EQ, "true").test(
          snap, record_buf(true, 0, 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("b", relop_id::LT, "c").test(
          snap, record_buf(false, 'a', 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("c", relop_id::LE, "10").test(
          snap, record_buf(false, 0, 10, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("d", relop_id::GT, "100").test(
          snap, record_buf(false, 0, 0, 101, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("e", relop_id::GE, "1000").test(
          snap, record_buf(false, 0, 0, 0, 1000, 0, 0)));

  ASSERT_TRUE(
      predicate("f", relop_id::NEQ, "100.3").test(
          snap, record_buf(false, 0, 0, 0, 0, 102.4, 0)));

  ASSERT_TRUE(
      predicate("g", relop_id::LT, "194.312").test(
          snap, record_buf(false, 0, 0, 0, 0, 0, 182.3)));
}

#endif /* TEST_COMPILED_PREDICATE_TEST_H_ */

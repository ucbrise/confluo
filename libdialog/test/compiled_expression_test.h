#ifndef TEST_COMPILED_EXPRESSION_TEST_H_
#define TEST_COMPILED_EXPRESSION_TEST_H_

#include "compiled_expression.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class CompiledExpressionTest : public testing::Test {
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
    predicate_t p;
    p.attr = attr;
    p.op = id;
    p.value = value;
    compiled_predicate c(p, s);
    return c;
  }
};

CompiledExpressionTest::rec CompiledExpressionTest::r;

schema_t CompiledExpressionTest::s = schema();

TEST_F(CompiledExpressionTest, TestCompiledExpressionRecordTest) {
  compiled_minterm m1, m2, m3;
  m1.add(predicate("a", relop_id::EQ, "true"));
  m1.add(predicate("b", relop_id::LT, "c"));

  m2.add(predicate("c", relop_id::LE, "10"));
  m2.add(predicate("d", relop_id::GT, "100"));

  m3.add(predicate("e", relop_id::GE, "1000"));
  m3.add(predicate("f", relop_id::NEQ, "100.3"));
  m3.add(predicate("g", relop_id::LT, "194.312"));

  compiled_expression cexp;
  cexp.insert(m1);
  cexp.insert(m2);
  cexp.insert(m3);

  ASSERT_TRUE(cexp.test(record(true, 'a', 10, 101, 1000, 102.4, 182.3)));
}

TEST_F(CompiledExpressionTest, TestCompiledExpressionBufferTest) {
  auto snap = s.snapshot();
  compiled_minterm m1, m2, m3;
  m1.add(predicate("a", relop_id::EQ, "true"));
  m1.add(predicate("b", relop_id::LT, "c"));

  m2.add(predicate("c", relop_id::LE, "10"));
  m2.add(predicate("d", relop_id::GT, "100"));

  m3.add(predicate("e", relop_id::GE, "1000"));
  m3.add(predicate("f", relop_id::NEQ, "100.3"));
  m3.add(predicate("g", relop_id::LT, "194.312"));

  compiled_expression cexp;
  cexp.insert(m1);
  cexp.insert(m2);
  cexp.insert(m3);

  ASSERT_TRUE(cexp.test(snap, record_buf(true, 'a', 10, 101, 1000, 102.4, 182.3)));
}

#endif /* TEST_COMPILED_EXPRESSION_TEST_H_ */

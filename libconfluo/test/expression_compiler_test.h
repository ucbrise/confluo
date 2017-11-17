#ifndef TEST_EXPRESSION_COMPILER_TEST_H_
#define TEST_EXPRESSION_COMPILER_TEST_H_

#include "parser/expression_compiler.h"

#include "gtest/gtest.h"
#include "schema.h"

using namespace ::confluo::parser;
using namespace ::confluo;

class ExpressionCompilerTest : public testing::Test {
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
    return s.apply_unsafe(0, &r);
  }

  static compiled_predicate predicate(const std::string& attr, reational_op_id id,
      const std::string& value) {
    return compiled_predicate(attr, id, value, s);
  }

  static void compile(compiled_expression& cexp, const std::string& exp,
      const schema_t& schema) {
    auto t = parse_expression(exp);
    cexp = compile_expression(t, schema);
  }

  static void check_predicate(const compiled_predicate& c) {
    if (c.to_string() == "A==bool(1)") {
      ASSERT_EQ("A", c.field_name());
      ASSERT_TRUE(BOOL_TYPE == c.value().type());
      ASSERT_EQ(reational_op_id::EQ, c.op());
    } else if (c.to_string() == "B<char(53)") {
      ASSERT_EQ("B", c.field_name());
      ASSERT_TRUE(CHAR_TYPE == c.value().type());
      ASSERT_EQ(reational_op_id::LT, c.op());
    } else if (c.to_string() == "C<short(10)") {
      ASSERT_EQ("C", c.field_name());
      ASSERT_TRUE(SHORT_TYPE == c.value().type());
      ASSERT_EQ(reational_op_id::LT, c.op());
    } else if (c.to_string() == "E<long(10)") {
      ASSERT_EQ("E", c.field_name());
      ASSERT_TRUE(LONG_TYPE == c.value().type());
      ASSERT_EQ(reational_op_id::LT, c.op());
    } else if (c.to_string() == "F<float(1.300000)") {
      ASSERT_EQ("F", c.field_name());
      ASSERT_TRUE(FLOAT_TYPE == c.value().type());
      ASSERT_EQ(reational_op_id::LT, c.op());
    } else if (c.to_string() == "G<double(1.900000)") {
      ASSERT_EQ("G", c.field_name());
      ASSERT_TRUE(DOUBLE_TYPE == c.value().type());
      ASSERT_EQ(reational_op_id::LT, c.op());
    } else {
      ASSERT_TRUE(false);
    }
  }
};

schema_t ExpressionCompilerTest::s = ExpressionCompilerTest::schema();
ExpressionCompilerTest::rec ExpressionCompilerTest::r;

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

TEST_F(ExpressionCompilerTest, TestCompiledExpressionRecordTest) {
  compiled_minterm m1, m2, m3;
  m1.add(predicate("a", reational_op_id::EQ, "true"));
  m1.add(predicate("b", reational_op_id::LT, "c"));

  m2.add(predicate("c", reational_op_id::LE, "10"));
  m2.add(predicate("d", reational_op_id::GT, "100"));

  m3.add(predicate("e", reational_op_id::GE, "1000"));
  m3.add(predicate("f", reational_op_id::NEQ, "100.3"));
  m3.add(predicate("g", reational_op_id::LT, "194.312"));

  compiled_expression cexp;
  cexp.insert(m1);
  cexp.insert(m2);
  cexp.insert(m3);

  ASSERT_TRUE(cexp.test(record(true, 'a', 10, 101, 1000, 102.4, 182.3)));
}

TEST_F(ExpressionCompilerTest, TestCompiledExpressionBufferTest) {
  auto snap = s.snapshot();
  compiled_minterm m1, m2, m3;
  m1.add(predicate("a", reational_op_id::EQ, "true"));
  m1.add(predicate("b", reational_op_id::LT, "c"));

  m2.add(predicate("c", reational_op_id::LE, "10"));
  m2.add(predicate("d", reational_op_id::GT, "100"));

  m3.add(predicate("e", reational_op_id::GE, "1000"));
  m3.add(predicate("f", reational_op_id::NEQ, "100.3"));
  m3.add(predicate("g", reational_op_id::LT, "194.312"));

  compiled_expression cexp;
  cexp.insert(m1);
  cexp.insert(m2);
  cexp.insert(m3);

  ASSERT_TRUE(
      cexp.test(snap, record_buf(true, 'a', 10, 101, 1000, 102.4, 182.3)));
}

TEST_F(ExpressionCompilerTest, TestMintermRecordTest) {
  compiled_minterm m1, m2, m3;
  m1.add(predicate("a", reational_op_id::EQ, "true"));
  m1.add(predicate("b", reational_op_id::LT, "c"));

  m2.add(predicate("c", reational_op_id::LE, "10"));
  m2.add(predicate("d", reational_op_id::GT, "100"));

  m3.add(predicate("e", reational_op_id::GE, "1000"));
  m3.add(predicate("f", reational_op_id::NEQ, "100.3"));
  m3.add(predicate("g", reational_op_id::LT, "194.312"));

  ASSERT_TRUE(m1.test(record(true, 'a', 11, 0, 0, 0.0, 0.0)));
  ASSERT_TRUE(m2.test(record(false, 'Z', 10, 101, 0, 0, 0)));
  ASSERT_TRUE(m3.test(record(false, 'Z', 11, 0, 1000, 102.4, 182.3)));
}

TEST_F(ExpressionCompilerTest, TestMintermBufferTest) {
  auto snap = s.snapshot();
  compiled_minterm m1, m2, m3;
  m1.add(predicate("a", reational_op_id::EQ, "true"));
  m1.add(predicate("b", reational_op_id::LT, "c"));

  m2.add(predicate("c", reational_op_id::LE, "10"));
  m2.add(predicate("d", reational_op_id::GT, "100"));

  m3.add(predicate("e", reational_op_id::GE, "1000"));
  m3.add(predicate("f", reational_op_id::NEQ, "100.3"));
  m3.add(predicate("g", reational_op_id::LT, "194.312"));

  ASSERT_TRUE(m1.test(snap, record_buf(true, 'a', 11, 0, 0, 0.0, 0.0)));
  ASSERT_TRUE(m2.test(snap, record_buf(false, 'Z', 10, 101, 0, 0, 0)));
  ASSERT_TRUE(m3.test(snap, record_buf(false, 'Z', 11, 0, 1000, 102.4, 182.3)));
}

TEST_F(ExpressionCompilerTest, TestPredicateRecordTest) {
  ASSERT_TRUE(
      predicate("a", reational_op_id::EQ, "true").test(
          record(true, 0, 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("b", reational_op_id::LT, "c").test(
          record(false, 'a', 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("c", reational_op_id::LE, "10").test(
          record(false, 0, 10, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("d", reational_op_id::GT, "100").test(
          record(false, 0, 0, 101, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("e", reational_op_id::GE, "1000").test(
          record(false, 0, 0, 0, 1000, 0, 0)));

  ASSERT_TRUE(
      predicate("f", reational_op_id::NEQ, "100.3").test(
          record(false, 0, 0, 0, 0, 102.4, 0)));

  ASSERT_TRUE(
      predicate("g", reational_op_id::LT, "194.312").test(
          record(false, 0, 0, 0, 0, 0, 182.3)));
}

TEST_F(ExpressionCompilerTest, TestPredicateBufferTest) {
  auto snap = s.snapshot();
  ASSERT_TRUE(
      predicate("a", reational_op_id::EQ, "true").test(
          snap, record_buf(true, 0, 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("b", reational_op_id::LT, "c").test(
          snap, record_buf(false, 'a', 0, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("c", reational_op_id::LE, "10").test(
          snap, record_buf(false, 0, 10, 0, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("d", reational_op_id::GT, "100").test(
          snap, record_buf(false, 0, 0, 101, 0, 0, 0)));

  ASSERT_TRUE(
      predicate("e", reational_op_id::GE, "1000").test(
          snap, record_buf(false, 0, 0, 0, 1000, 0, 0)));

  ASSERT_TRUE(
      predicate("f", reational_op_id::NEQ, "100.3").test(
          snap, record_buf(false, 0, 0, 0, 0, 102.4, 0)));

  ASSERT_TRUE(
      predicate("g", reational_op_id::LT, "194.312").test(
          snap, record_buf(false, 0, 0, 0, 0, 0, 182.3)));
}

#endif // TEST_EXPRESSION_COMPILER_TEST_H_

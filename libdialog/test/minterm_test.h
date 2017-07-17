#ifndef TEST_MINTERM_TEST_H_
#define TEST_MINTERM_TEST_H_

#include "minterm.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class MintermTest : public testing::Test {
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

  struct ts_rec {
    uint64_t ts;
    struct rec r;
  }__attribute__((packed));

  static ts_rec r;

  static schema_t<storage::in_memory> schema() {
    schema_builder builder;
    builder.add_column(BOOL_TYPE, "a");
    builder.add_column(CHAR_TYPE, "b");
    builder.add_column(SHORT_TYPE, "c");
    builder.add_column(INT_TYPE, "d");
    builder.add_column(LONG_TYPE, "e");
    builder.add_column(FLOAT_TYPE, "f");
    builder.add_column(DOUBLE_TYPE, "g");
    builder.add_column(STRING_TYPE(16), "h");
    return schema_t<storage::in_memory>(".", builder.get_columns());
  }

  record_t record(bool a, char b, short c, int d, long e, float f, double g) {
    r = {0, {a, b, c, d, e, f, g, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          0, 0}}};
    return s.apply(0, &r, sizeof(rec));
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

MintermTest::ts_rec MintermTest::r;

schema_t<storage::in_memory> MintermTest::s = schema();

TEST_F(MintermTest, TestMintermTest) {
  minterm m1, m2, m3;
  m1.add(predicate("a", relop_id::EQ, "true"));
  m1.add(predicate("b", relop_id::LT, "c"));

  m2.add(predicate("c", relop_id::LE, "10"));
  m2.add(predicate("d", relop_id::GT, "100"));

  m3.add(predicate("e", relop_id::GE, "1000"));
  m3.add(predicate("f", relop_id::NEQ, "100.3"));
  m3.add(predicate("g", relop_id::LT, "194.312"));

  ASSERT_TRUE(m1.test(record(true, 'a', 11, 0, 0, 0.0, 0.0)));
  ASSERT_TRUE(m2.test(record(false, 'Z', 10, 101, 0, 0, 0)));
  ASSERT_TRUE(m3.test(record(false, 'Z', 11, 0, 1000, 102.4, 182.3)));
}

#endif /* TEST_MINTERM_TEST_H_ */

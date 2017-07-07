#ifndef LIBDIALOG_TEST_MINTERM_TEST_H_
#define LIBDIALOG_TEST_MINTERM_TEST_H_

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
  };

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
    return schema_t<storage::in_memory>(".", builder.get_schema());
  }

  record_t record(bool a, char b, short c, int d, long e, float f, double g) {
    rec r = { a, b, c, d, e, f, g, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0 } };
    return s.apply(0, &r, sizeof(rec), 0);
  }

  static compiled_predicate predicate(const std::string& attr, relop_id id,
                                      const std::string& value) {
    predicate_t p;
    p.attr = attr;
    p.op = id;
    p.value = value;
    return compiled_predicate(p, s);
  }
};

schema_t<storage::in_memory> MintermTest::s = schema();

TEST_F(MintermTest, TestMintermTest) {
  minterm m;
  m.add(predicate("a", relop_id::EQ, "true"));
  m.add(predicate("b", relop_id::LT, "c"));

  m.add(predicate("c", relop_id::LE, "10"));
  m.add(predicate("d", relop_id::GT, "100"));

  m.add(predicate("e", relop_id::GE, "1000"));
  m.add(predicate("f", relop_id::NEQ, "100.3"));
  m.add(predicate("g", relop_id::LT, "194.312"));

  m.test(record(true, 'a', 11, 0, 0, 0.0, 0.0));
  m.test(record(false, 'Z', 10, 101, 0, 0, 0));
  m.test(record(false, 'Z', 11, 0, 1000, 102.4, 182.3));
}

#endif /* LIBDIALOG_TEST_MINTERM_TEST_H_ */

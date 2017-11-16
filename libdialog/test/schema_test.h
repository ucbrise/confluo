#ifndef TEST_SCHEMA_TEST_H_
#define TEST_SCHEMA_TEST_H_

#include "schema.h"
#include "gtest/gtest.h"

using namespace ::confluo;

class SchemaTest : public testing::Test {
 public:
  struct rec {
    rec(bool _a, int8_t _b, int16_t _c, int32_t _d, int64_t _e, float _f,
        double _g)
        : ts(0),
          a(_a),
          b(_b),
          c(_c),
          d(_d),
          e(_e),
          f(_f),
          g(_g) {
    }

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
};

TEST_F(SchemaTest, SchemaBuilderTest) {
  schema_builder builder;
  builder.add_column(BOOL_TYPE, "a");
  builder.add_column(CHAR_TYPE, "b");
  builder.add_column(SHORT_TYPE, "c");
  builder.add_column(INT_TYPE, "d");
  builder.add_column(LONG_TYPE, "e");
  builder.add_column(FLOAT_TYPE, "f");
  builder.add_column(DOUBLE_TYPE, "g");
  builder.add_column(STRING_TYPE(16), "h");
  auto schema_vec = builder.get_columns();

  ASSERT_EQ(static_cast<size_t>(9), schema_vec.size());

  ASSERT_EQ(0, schema_vec[0].idx());
  ASSERT_EQ(0, schema_vec[0].offset());
  ASSERT_EQ("TIMESTAMP", schema_vec[0].name());
  ASSERT_TRUE(LONG_TYPE == schema_vec[0].type());
  ASSERT_FALSE(schema_vec[0].is_indexed());

  ASSERT_EQ(1, schema_vec[1].idx());
  ASSERT_EQ(8, schema_vec[1].offset());
  ASSERT_EQ("A", schema_vec[1].name());
  ASSERT_TRUE(BOOL_TYPE == schema_vec[1].type());
  ASSERT_FALSE(schema_vec[1].is_indexed());

  ASSERT_EQ(2, schema_vec[2].idx());
  ASSERT_EQ(9, schema_vec[2].offset());
  ASSERT_EQ("B", schema_vec[2].name());
  ASSERT_TRUE(CHAR_TYPE == schema_vec[2].type());
  ASSERT_FALSE(schema_vec[2].is_indexed());

  ASSERT_EQ(3, schema_vec[3].idx());
  ASSERT_EQ(10, schema_vec[3].offset());
  ASSERT_EQ("C", schema_vec[3].name());
  ASSERT_TRUE(SHORT_TYPE == schema_vec[3].type());
  ASSERT_FALSE(schema_vec[3].is_indexed());

  ASSERT_EQ(4, schema_vec[4].idx());
  ASSERT_EQ(12, schema_vec[4].offset());
  ASSERT_EQ("D", schema_vec[4].name());
  ASSERT_TRUE(INT_TYPE == schema_vec[4].type());
  ASSERT_FALSE(schema_vec[4].is_indexed());

  ASSERT_EQ(5, schema_vec[5].idx());
  ASSERT_EQ(16, schema_vec[5].offset());
  ASSERT_EQ("E", schema_vec[5].name());
  ASSERT_TRUE(LONG_TYPE == schema_vec[5].type());
  ASSERT_FALSE(schema_vec[5].is_indexed());

  ASSERT_EQ(6, schema_vec[6].idx());
  ASSERT_EQ(24, schema_vec[6].offset());
  ASSERT_EQ("F", schema_vec[6].name());
  ASSERT_TRUE(FLOAT_TYPE == schema_vec[6].type());
  ASSERT_FALSE(schema_vec[6].is_indexed());

  ASSERT_EQ(7, schema_vec[7].idx());
  ASSERT_EQ(28, schema_vec[7].offset());
  ASSERT_EQ("G", schema_vec[7].name());
  ASSERT_TRUE(DOUBLE_TYPE == schema_vec[7].type());
  ASSERT_FALSE(schema_vec[7].is_indexed());

  ASSERT_EQ(8, schema_vec[8].idx());
  ASSERT_EQ(36, schema_vec[8].offset());
  ASSERT_EQ("H", schema_vec[8].name());
  ASSERT_TRUE(STRING_TYPE(16) == schema_vec[8].type());
  ASSERT_FALSE(schema_vec[8].is_indexed());
}

TEST_F(SchemaTest, SchemaOpsTest) {
  schema_builder builder;
  builder.add_column(BOOL_TYPE, "a");
  builder.add_column(CHAR_TYPE, "b");
  builder.add_column(SHORT_TYPE, "c");
  builder.add_column(INT_TYPE, "d");
  builder.add_column(LONG_TYPE, "e");
  builder.add_column(FLOAT_TYPE, "f");
  builder.add_column(DOUBLE_TYPE, "g");
  builder.add_column(STRING_TYPE(16), "h");
  auto schema_vec = builder.get_columns();
  schema_t s(schema_vec);

  ASSERT_EQ(static_cast<size_t>(9), s.size());

  ASSERT_EQ(0, s[0].idx());
  ASSERT_EQ(0, s[0].offset());
  ASSERT_EQ("TIMESTAMP", s[0].name());
  ASSERT_TRUE(LONG_TYPE == s[0].type());
  ASSERT_FALSE(s[0].is_indexed());

  ASSERT_EQ(1, s[1].idx());
  ASSERT_EQ(8, s[1].offset());
  ASSERT_EQ("A", s[1].name());
  ASSERT_TRUE(BOOL_TYPE == s[1].type());
  ASSERT_FALSE(s[1].is_indexed());

  ASSERT_EQ(2, s[2].idx());
  ASSERT_EQ(9, s[2].offset());
  ASSERT_EQ("B", s[2].name());
  ASSERT_TRUE(CHAR_TYPE == s[2].type());
  ASSERT_FALSE(s[2].is_indexed());

  ASSERT_EQ(3, s[3].idx());
  ASSERT_EQ(10, s[3].offset());
  ASSERT_EQ("C", s[3].name());
  ASSERT_TRUE(SHORT_TYPE == s[3].type());
  ASSERT_FALSE(s[3].is_indexed());

  ASSERT_EQ(4, s[4].idx());
  ASSERT_EQ(12, s[4].offset());
  ASSERT_EQ("D", s[4].name());
  ASSERT_TRUE(INT_TYPE == s[4].type());
  ASSERT_FALSE(s[4].is_indexed());

  ASSERT_EQ(5, s[5].idx());
  ASSERT_EQ(16, s[5].offset());
  ASSERT_EQ("E", s[5].name());
  ASSERT_TRUE(LONG_TYPE == s[5].type());
  ASSERT_FALSE(s[5].is_indexed());

  ASSERT_EQ(6, s[6].idx());
  ASSERT_EQ(24, s[6].offset());
  ASSERT_EQ("F", s[6].name());
  ASSERT_TRUE(FLOAT_TYPE == s[6].type());
  ASSERT_FALSE(s[6].is_indexed());

  ASSERT_EQ(7, s[7].idx());
  ASSERT_EQ(28, s[7].offset());
  ASSERT_EQ("G", s[7].name());
  ASSERT_TRUE(DOUBLE_TYPE == s[7].type());
  ASSERT_FALSE(s[7].is_indexed());

  ASSERT_EQ(8, s[8].idx());
  ASSERT_EQ(36, s[8].offset());
  ASSERT_EQ("H", s[8].name());
  ASSERT_TRUE(STRING_TYPE(16) == s[8].type());
  ASSERT_FALSE(s[8].is_indexed());

  ASSERT_EQ(0, s["TIMESTAMP"].idx());
  ASSERT_EQ(0, s["TIMESTAMP"].offset());
  ASSERT_EQ("TIMESTAMP", s["TIMESTAMP"].name());
  ASSERT_TRUE(LONG_TYPE == s["TIMESTAMP"].type());
  ASSERT_FALSE(s["TIMESTAMP"].is_indexed());

  ASSERT_EQ(1, s["A"].idx());
  ASSERT_EQ(8, s["A"].offset());
  ASSERT_EQ("A", s["A"].name());
  ASSERT_TRUE(BOOL_TYPE == s["A"].type());
  ASSERT_FALSE(s["A"].is_indexed());

  ASSERT_EQ(2, s["B"].idx());
  ASSERT_EQ(9, s["B"].offset());
  ASSERT_EQ("B", s["B"].name());
  ASSERT_TRUE(CHAR_TYPE == s["B"].type());
  ASSERT_FALSE(s["B"].is_indexed());

  ASSERT_EQ(3, s["C"].idx());
  ASSERT_EQ(10, s["C"].offset());
  ASSERT_EQ("C", s["C"].name());
  ASSERT_TRUE(SHORT_TYPE == s["C"].type());
  ASSERT_FALSE(s["C"].is_indexed());

  ASSERT_EQ(4, s["D"].idx());
  ASSERT_EQ(12, s["D"].offset());
  ASSERT_EQ("D", s["D"].name());
  ASSERT_TRUE(INT_TYPE == s["D"].type());
  ASSERT_FALSE(s["D"].is_indexed());

  ASSERT_EQ(5, s["E"].idx());
  ASSERT_EQ(16, s["E"].offset());
  ASSERT_EQ("E", s["E"].name());
  ASSERT_TRUE(LONG_TYPE == s["E"].type());
  ASSERT_FALSE(s["E"].is_indexed());

  ASSERT_EQ(6, s["F"].idx());
  ASSERT_EQ(24, s["F"].offset());
  ASSERT_EQ("F", s["F"].name());
  ASSERT_TRUE(FLOAT_TYPE == s["F"].type());
  ASSERT_FALSE(s["F"].is_indexed());

  ASSERT_EQ(7, s["G"].idx());
  ASSERT_EQ(28, s["G"].offset());
  ASSERT_EQ("G", s["G"].name());
  ASSERT_TRUE(DOUBLE_TYPE == s["G"].type());
  ASSERT_FALSE(s["G"].is_indexed());

  ASSERT_EQ(8, s["H"].idx());
  ASSERT_EQ(36, s["H"].offset());
  ASSERT_EQ("H", s["H"].name());
  ASSERT_TRUE(STRING_TYPE(16) == s["H"].type());
  ASSERT_FALSE(s["H"].is_indexed());
}

TEST_F(SchemaTest, SchemaApplyTest) {
  schema_builder builder;
  builder.add_column(BOOL_TYPE, "a");
  builder.add_column(CHAR_TYPE, "b");
  builder.add_column(SHORT_TYPE, "c");
  builder.add_column(INT_TYPE, "d");
  builder.add_column(LONG_TYPE, "e");
  builder.add_column(FLOAT_TYPE, "f");
  builder.add_column(DOUBLE_TYPE, "g");
  builder.add_column(STRING_TYPE(16), "h");
  auto schema_vec = builder.get_columns();
  schema_t s(schema_vec);

  rec tmp(true, 'a', 10, 101, 1000, 102.4, 182.3);
  record_t r = s.apply_unsafe(0, &tmp);

  ASSERT_EQ(0, r[0].idx());
  ASSERT_EQ(tmp.ts, r[0].as<int64_t>());
  ASSERT_TRUE(LONG_TYPE == r[0].type());
  ASSERT_FALSE(r[0].is_indexed());

  ASSERT_EQ(1, r[1].idx());
  ASSERT_EQ(tmp.a, r[1].as<bool>());
  ASSERT_TRUE(BOOL_TYPE == r[1].type());
  ASSERT_FALSE(r[1].is_indexed());

  ASSERT_EQ(2, r[2].idx());
  ASSERT_EQ(tmp.b, r[2].as<int8_t>());
  ASSERT_TRUE(CHAR_TYPE == r[2].type());
  ASSERT_FALSE(r[2].is_indexed());

  ASSERT_EQ(3, r[3].idx());
  ASSERT_EQ(tmp.c, r[3].as<int16_t>());
  ASSERT_TRUE(SHORT_TYPE == r[3].type());
  ASSERT_FALSE(r[3].is_indexed());

  ASSERT_EQ(4, r[4].idx());
  ASSERT_EQ(tmp.d, r[4].as<int32_t>());
  ASSERT_TRUE(INT_TYPE == r[4].type());
  ASSERT_FALSE(r[4].is_indexed());

  ASSERT_EQ(5, r[5].idx());
  ASSERT_EQ(tmp.e, r[5].as<int64_t>());
  ASSERT_TRUE(LONG_TYPE == r[5].type());
  ASSERT_FALSE(r[5].is_indexed());

  ASSERT_EQ(6, r[6].idx());
  ASSERT_EQ(tmp.f, r[6].as<float>());
  ASSERT_TRUE(FLOAT_TYPE == r[6].type());
  ASSERT_FALSE(r[6].is_indexed());

  ASSERT_EQ(7, r[7].idx());
  ASSERT_TRUE(DOUBLE_TYPE == r[7].type());
  ASSERT_EQ(tmp.g, r[7].as<double>());
  ASSERT_FALSE(r[7].is_indexed());

  ASSERT_EQ(8, r[8].idx());
  ASSERT_TRUE(STRING_TYPE(16) == r[8].type());
  ASSERT_FALSE(r[8].is_indexed());
}

#endif /* TEST_SCHEMA_TEST_H_ */

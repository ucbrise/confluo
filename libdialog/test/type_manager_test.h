#ifndef TEST_TYPE_MANAGER_TEST_H_
#define TEST_TYPE_MANAGER_TEST_H_

#include "types/type_manager.h"

#include "ip_address.h"
#include "size_type.h"
#include "dialog_table.h"
#include "parser/schema_parser.h"
#include "gtest/gtest.h"
#include <regex>
#include <fstream>

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::confluo;

type_properties ip_type_properties("ip_address", sizeof(ip_address),
                                   &limits::int_min, &limits::int_max,
                                   &limits::int_one, &limits::int_zero, false,
                                   get_relops(), get_unaryops(),
                                   get_binaryops(), get_keyops(),
                                   &ip_address::parse_ip,
                                   &ip_address::ip_to_string,
                                   &confluo::serialize<ip_address>,
                                   &confluo::deserialize<ip_address>);

type_properties size_type_properties("size_type", sizeof(size_type),
                                     &limits::ulong_min,
                                     &limits::ulong_max,
                                     &limits::ulong_one,
                                     &limits::ulong_zero, true, get_reops(),
                                     get_unarops(), get_binarops(), get_keops(),
                                     &size_type::parse_bytes,
                                     &size_type::size_to_string,
                                     &confluo::serialize<size_type>,
                                     &confluo::deserialize<size_type>);

class TypeManagerTest : public testing::Test {
 public:
  static std::vector<column_t> schema() {
    type_manager::register_type(ip_type_properties);
    type_manager::register_type(size_type_properties);

    addr_type = type_manager::get_type("ip_address");
    sz_type = type_manager::get_type("size_type");

    schema_builder builder;
    builder.add_column(addr_type, "a");
    builder.add_column(addr_type, "b");
    builder.add_column(sz_type, "c");

    return builder.get_columns();
  }

  static task_pool MGMT_POOL;
  static std::vector<column_t> s;
  static data_type addr_type;
  static data_type sz_type;

  static void generate_bytes(uint8_t* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++) {
      buf[i] = val_uint8;
    }
  }

  struct rec {
    int64_t ts;
    ip_address a;
    ip_address b;
    size_type c;
  }__attribute__((packed));

  static rec r;

  static void* record(ip_address a, ip_address b, size_type c) {
    int64_t ts = utils::time_utils::cur_ns();
    r = {ts, a, b, c};
    return reinterpret_cast<void*>(&r);
  }

  static void compile(compiled_expression& cexp, const std::string& exp,
                      const schema_t& schema) {
    auto t = parse_expression(exp);
    cexp = compile_expression(t, schema);
  }

  static compiled_predicate predicate(const std::string& attr,
                                      reational_op_id id,
                                      const std::string& value) {
    return compiled_predicate(attr, id, value, schema());
  }

 protected:
  uint8_t data_[DATA_SIZE];

  virtual void SetUp() override {
    thread_manager::register_thread();
  }

  virtual void TearDown() override {
    thread_manager::deregister_thread();
  }
};

TypeManagerTest::rec TypeManagerTest::r;
data_type TypeManagerTest::addr_type;
data_type TypeManagerTest::sz_type;
std::vector<column_t> TypeManagerTest::s = schema();
task_pool TypeManagerTest::MGMT_POOL;

TEST_F(TypeManagerTest, RegisterTest) {
  ASSERT_EQ(limits::int_one, *reinterpret_cast<int*>(addr_type.one()));
  ASSERT_EQ(limits::int_min, *reinterpret_cast<int*>(addr_type.min()));
  ASSERT_EQ(limits::int_max, *reinterpret_cast<int*>(addr_type.max()));
  ASSERT_EQ(limits::int_zero, *reinterpret_cast<int*>(addr_type.zero()));
  ASSERT_STREQ("ip_address", addr_type.name().c_str());
  ASSERT_EQ(9, confluo::type_manager::get_type("ip_address").id);
  ASSERT_STREQ("ip_address", s[1].type().name().c_str());

  ASSERT_STREQ("size_type", sz_type.name().c_str());
  ASSERT_EQ(10, confluo::type_manager::get_type("size_type").id);
  ASSERT_STREQ("size_type", s[3].type().name().c_str());

}

TEST_F(TypeManagerTest, FilterTest) {
  dialog_table dtable("my_table", s, "/tmp", storage::IN_MEMORY, MGMT_POOL);
  dtable.add_index("a");
  dtable.add_index("b");
  dtable.add_index("c");

  dtable.append(
      record(ip_address(52), ip_address(42), size_type::from_string("1024b")));
  dtable.append(
      record(ip_address(50), ip_address(300), size_type::from_string("1kb")));

  size_t i = 0;
  for (auto r = dtable.execute_filter("a > 0.0.0.3"); !r.empty(); r =
      r.tail()) {
    ASSERT_TRUE(r.head().at(1).value().as<ip_address>().get_address() > 33);
    i++;
  }
  ASSERT_EQ(2, i);

  i = 0;
  for (auto r = dtable.execute_filter("c == 1kb"); !r.empty(); r = r.tail()) {
    ASSERT_TRUE(
        r.head().at(3).value().as<size_type>().get_bytes()
            == static_cast<uint64_t>(1024));
    i++;
  }

  ASSERT_EQ(2, i);
}

TEST_F(TypeManagerTest, IPAddressTest) {
  mutable_raw_data d1(addr_type.size), d2(addr_type.size), d3(addr_type.size);
  ip_address::parse_ip("69.89.31.226", d1);
  ip_address::parse_ip("216.65.216.164", d2);
  ip_address::parse_ip("172.16.254.1", d3);

  mutable_value n1(addr_type, std::move(d1));
  mutable_value n2(addr_type, std::move(d2));
  mutable_value n3(addr_type, std::move(d3));

  ASSERT_EQ(9, n1.type().id);
  ASSERT_EQ(9, n2.type().id);
  ASSERT_EQ(9, n3.type().id);

  ASSERT_EQ(1163468770, n1.as<ip_address>().get_address());
  ASSERT_EQ(3628193956, n2.as<ip_address>().get_address());
  ASSERT_EQ(2886794753, n3.as<ip_address>().get_address());

  ASSERT_TRUE(n3 > n1);
  ASSERT_TRUE(n2 > n3);
  ASSERT_TRUE(n2 > n1);

  ASSERT_FALSE(n1 > n3);
  ASSERT_FALSE(n3 > n2);
  ASSERT_FALSE(n1 > n2);

  mutable_raw_data d4 = ip_address::parse_ip_value(UINT32_C(1163468770));
  mutable_raw_data d5 = ip_address::parse_ip_value(UINT32_C(3628193956));
  mutable_raw_data d6 = ip_address::parse_ip_value(
      static_cast<uint32_t>(4791662726));

  ip_address addr(1163468770);
  ASSERT_TRUE(mutable_value(n1.type(), &addr) == n1);
  ASSERT_TRUE(mutable_value(n1.type(), std::move(d4)) == n1);
  ASSERT_TRUE(mutable_value(n2.type(), std::move(d5)) == n2);
  ASSERT_TRUE(mutable_value(n3.type(), std::move(d6)) == (n1 + n2));

  mutable_value val1 = mutable_value::parse("42.35.109.253", n1.type());
  ASSERT_EQ(706964989, val1.as<ip_address>().get_address());

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("-1.3.4.5", addr_type);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("278.1.1.1", addr_type);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("123412341", addr_type);
  }));
}

TEST_F(TypeManagerTest, SizeTypeTest) {
  mutable_raw_data d1(sz_type.size), d2(sz_type.size), d3(sz_type.size);
  size_type::parse_bytes("3mb", d1);
  size_type::parse_bytes("10gb", d2);
  size_type::parse_bytes("3072kb", d3);

  mutable_value n1(sz_type, std::move(d1));
  mutable_value n2(sz_type, std::move(d2));
  mutable_value n3(sz_type, std::move(d3));

  ASSERT_EQ(10, n1.type().id);
  ASSERT_EQ(10, n2.type().id);
  ASSERT_EQ(10, n3.type().id);

  ASSERT_TRUE(3145728 == n1.as<size_type>().get_bytes());
  ASSERT_TRUE(10737418240 == n2.as<size_type>().get_bytes());
  ASSERT_TRUE(3145728 == n3.as<size_type>().get_bytes());

  ASSERT_TRUE(n2 > n1);
  ASSERT_TRUE(n2 > n3);
  ASSERT_TRUE(n3 == n1);

  ASSERT_FALSE(n1 > n2);
  ASSERT_FALSE(n3 > n2);
  ASSERT_FALSE(n1 > n3);

  mutable_raw_data d4(sz_type.size), d5(sz_type.size), d6(sz_type.size);
  size_type::parse_bytes("3145728b", d4);
  size_type::parse_bytes("10737418240b", d5);
  size_type::parse_bytes("10740563968b", d6);

  size_type sz(3145728);
  ASSERT_TRUE(mutable_value(n1.type(), &sz) == n1);
  ASSERT_TRUE(mutable_value(n1.type(), std::move(d4)) == n1);
  ASSERT_TRUE(mutable_value(n2.type(), std::move(d5)) == n2);
  ASSERT_TRUE(mutable_value(n3.type(), std::move(d6)) == (n1 + n2));

  mutable_value val1 = mutable_value::parse("15pb", n1.type());
  mutable_raw_data d7(sz_type.size);
  size_type::parse_bytes("16888498602639360b", d7);

  ASSERT_TRUE(16888498602639360 == val1.as<size_type>().get_bytes());

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("234xb", addr_type);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("1234k", addr_type);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("10987", addr_type);
  }));
}

TEST_F(TypeManagerTest, CompilerTest) {
  compiled_expression m1, m2, m3;
  compile(m1, "a==192.34.123.4", s);
  ASSERT_EQ(static_cast<size_t>(1), m1.size());

  compile(m2, "b==123.43.234.64", s);
  ASSERT_EQ(static_cast<size_t>(1), m2.size());

  compile(m3, "c==67kb", s);
  ASSERT_EQ(static_cast<size_t>(1), m3.size());

  compiled_minterm m4, m5, m6;
  m4.add(predicate("a", reational_op_id::EQ, "4.4.4.4"));
  m5.add(predicate("b", reational_op_id::GT, "0.0.0.7"));
  m6.add(predicate("c", reational_op_id::EQ, "4kb"));

  compiled_expression cexp;
  cexp.insert(m4);
  cexp.insert(m5);
  cexp.insert(m6);

  ASSERT_STREQ(
      "A==ip_address(4.4.4.4) or B>ip_address(0.0.0.7)C==size_type(4096b)",
      cexp.to_string().c_str());

}

TEST_F(TypeManagerTest, SchemaTest) {
  schema_builder build;
  build.add_column(addr_type, "a");
  build.add_column(addr_type, "b");
  build.add_column(sz_type, "c");
  build.add_column(sz_type, "d");
  auto schema_vec = build.get_columns();
  schema_t s(schema_vec);

  ASSERT_EQ(static_cast<size_t>(5), schema_vec.size());
  ASSERT_EQ(0, schema_vec[0].idx());
  ASSERT_EQ(0, schema_vec[0].offset());
  ASSERT_EQ("TIMESTAMP", schema_vec[0].name());
  ASSERT_TRUE(LONG_TYPE == schema_vec[0].type());
  ASSERT_FALSE(schema_vec[0].is_indexed());

  ASSERT_EQ(1, schema_vec[1].idx());
  ASSERT_EQ(8, schema_vec[1].offset());
  ASSERT_EQ("A", schema_vec[1].name());
  ASSERT_TRUE(addr_type == schema_vec[1].type());
  ASSERT_FALSE(schema_vec[1].is_indexed());

  ASSERT_EQ(2, s[2].idx());
  ASSERT_EQ(12, s[2].offset());
  ASSERT_EQ("B", s[2].name());
  ASSERT_TRUE(addr_type == s[2].type());
  ASSERT_FALSE(s[2].is_indexed());

  ASSERT_EQ(3, s[3].idx());
  ASSERT_EQ(16, s[3].offset());
  ASSERT_EQ("C", s[3].name());
  ASSERT_TRUE(sz_type == s[3].type());
  ASSERT_FALSE(s[3].is_indexed());

  ASSERT_EQ(4, s[4].idx());
  ASSERT_EQ(24, s[4].offset());
  ASSERT_EQ("D", s[4].name());
  ASSERT_TRUE(sz_type == s[4].type());
  ASSERT_FALSE(s[4].is_indexed());

  ASSERT_EQ(static_cast<size_t>(5), s.size());

  ASSERT_EQ(0, s[0].idx());
  ASSERT_EQ(0, s[0].offset());
  ASSERT_EQ("TIMESTAMP", s[0].name());
  ASSERT_TRUE(LONG_TYPE == s[0].type());
  ASSERT_FALSE(s[0].is_indexed());

  ASSERT_EQ(1, s[1].idx());
  ASSERT_EQ(8, s[1].offset());
  ASSERT_EQ("A", s[1].name());
  ASSERT_TRUE(addr_type == s[1].type());
  ASSERT_FALSE(s[1].is_indexed());

  ASSERT_EQ(2, s[2].idx());
  ASSERT_EQ(12, s[2].offset());
  ASSERT_EQ("B", s[2].name());
  ASSERT_TRUE(addr_type == s[2].type());
  ASSERT_FALSE(s[2].is_indexed());

  ASSERT_EQ(3, s[3].idx());
  ASSERT_EQ(16, s[3].offset());
  ASSERT_EQ("C", s[3].name());
  ASSERT_TRUE(sz_type == s[3].type());
  ASSERT_FALSE(s[3].is_indexed());

  ASSERT_EQ(4, s[4].idx());
  ASSERT_EQ(24, s[4].offset());
  ASSERT_EQ("D", s[4].name());
  ASSERT_TRUE(sz_type == s[4].type());
  ASSERT_FALSE(s[4].is_indexed());
}

TEST_F(TypeManagerTest, DataTypesGetterTest) {
  ASSERT_TRUE(type_manager::get_type("ip_address") == addr_type);
  ASSERT_TRUE(type_manager::get_type("size_type") == sz_type);
  ASSERT_FALSE(addr_type == sz_type);

  ASSERT_TRUE(addr_type.name() == "ip_address");
  ASSERT_TRUE(sz_type.name() == "size_type");
}

TEST_F(TypeManagerTest, SerializeTest) {
  std::ofstream outfile("/tmp/test.txt", std::ofstream::binary);
  int32_t val = 390;
  immutable_raw_data d1(&val, INT_TYPE.size);
  mutable_raw_data d11(addr_type.size);
  ip_address::parse_ip("69.89.31.226", d11);

  ASSERT_EQ(390, d1.as<int>());
  INT_TYPE.serialize_op()(outfile, d1);
  addr_type.serialize_op()(outfile, d11.immutable());
  outfile.close();

  std::ifstream infile("/tmp/test.txt", std::ifstream::binary);
  int32_t val_read;
  infile.read(reinterpret_cast<char*>(&val_read), INT_TYPE.size);
  ASSERT_EQ(390, val_read);

  ip_address addr_read;
  infile.read(reinterpret_cast<char*>(&addr_read), addr_type.size);
  ASSERT_EQ(1163468770, addr_read.get_address());

  infile.close();
}

TEST_F(TypeManagerTest, DeserializeTest) {
  std::ofstream outfile("/tmp/test1.txt", std::ofstream::binary);

  int val = -490;
  immutable_raw_data d1 = immutable_raw_data(&val, INT_TYPE.size);
  mutable_raw_data d11(addr_type.size);
  ip_address::parse_ip("69.89.31.226", d11);

  ASSERT_EQ(-490, d1.as<int>());

  INT_TYPE.serialize_op()(outfile, d1);
  addr_type.serialize_op()(outfile, d11.immutable());
  outfile.close();

  std::ifstream infile("/tmp/test1.txt", std::ifstream::binary);
  mutable_raw_data d3(INT_TYPE.size);
  INT_TYPE.deserialize_op()(infile, d3);
  ASSERT_EQ(-490, d3.as<int32_t>());

  mutable_raw_data d13(addr_type.size);
  addr_type.deserialize_op()(infile, d13);
  ASSERT_EQ(1163468770, d13.as<ip_address>().get_address());

  infile.close();
}

#endif /* TEST_TYPE_MANAGER_TEST_H_ */

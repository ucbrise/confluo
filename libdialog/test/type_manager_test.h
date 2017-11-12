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

using namespace ::dialog;

type_properties type_def(sizeof(uint32_t), get_relops(), get_unaryops(),
                         get_binaryops(), get_keyops(), &limits::int_min,
                         &limits::int_max, &limits::int_one, &limits::int_zero,
                         &ip_address::to_string, &ip_address::parse_ip,
                         &dialog::serialize<ip_address>,
                         &dialog::deserialize<ip_address>);

type_properties size_type_ops(sizeof(uint64_t), get_reops(), get_unarops(),
                              get_binarops(), get_keops(),
                              &limits::long_long_min, &limits::long_long_max,
                              &limits::long_long_one, &limits::long_long_zero,
                              &size_type::to_string, &size_type::parse_bytes,
                              &dialog::serialize<size_type>,
                              &dialog::deserialize<size_type>);

class TypeManagerTest : public testing::Test {
 public:
  static std::vector<column_t> schema() {
    //type_manager::register_primitives();
    type_manager::register_type(type_def);
    type_manager::register_type(size_type_ops);

    schema_builder builder;
    builder.add_column(data_types[9], "a");
    builder.add_column(data_types[9], "b");
    builder.add_column(
        data_types[type_manager::get_id_from_type_name("size type")], "c");

    return builder.get_columns();
  }

  static task_pool MGMT_POOL;
  static std::vector<column_t> s;

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

  static compiled_predicate predicate(const std::string& attr, relop_id id,
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
std::vector<column_t> TypeManagerTest::s = schema();
task_pool TypeManagerTest::MGMT_POOL;

TEST_F(TypeManagerTest, RegisterTest) {
  int* limit_reference = (int*) (ONE[9]);
  int limit = *limit_reference;
  ASSERT_EQ(limits::int_one, limit);

  int* min_ref = (int*) MIN[9];
  int min = *min_ref;
  ASSERT_EQ(limits::int_min, min);

  int* max_ref = (int*) MAX[9];
  int max = *max_ref;
  ASSERT_EQ(limits::int_max, max);

  int* zero_ref = (int*) ZERO[9];
  int zero = *zero_ref;
  ASSERT_EQ(limits::int_zero, zero);

  ASSERT_STREQ("ip_address", data_types[9].to_string().c_str());
  ASSERT_EQ(9, dialog::type_manager::get_id_from_type_name("ip_address"));
  ASSERT_STREQ("ip_address", s[1].type().to_string().c_str());

  ASSERT_STREQ("size type", data_types[10].to_string().c_str());
  ASSERT_EQ(10, dialog::type_manager::get_id_from_type_name("size type"));
  ASSERT_STREQ("size type", s[3].type().to_string().c_str());

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
  data d1 = ip_address::parse_ip("69.89.31.226");
  data d2 = ip_address::parse_ip("216.65.216.164");
  data d3 = ip_address::parse_ip("172.16.254.1");

  mutable_value n1(
      data_types[type_manager::get_id_from_type_name("ip_address")], d1);
  mutable_value n2(
      data_types[type_manager::get_id_from_type_name("ip_address")], d2);
  mutable_value n3(
      data_types[type_manager::get_id_from_type_name("ip_address")], d3);

  ASSERT_EQ(9, n1.type().id);
  ASSERT_EQ(9, n2.type().id);
  ASSERT_EQ(9, n3.type().id);

  ASSERT_EQ(1163468770,
            (*reinterpret_cast<const ip_address*>(n1.ptr())).get_address());
  ASSERT_EQ(3628193956,
            (*reinterpret_cast<const ip_address*>(n2.ptr())).get_address());
  ASSERT_EQ(2886794753,
            (*reinterpret_cast<const ip_address*>(n3.ptr())).get_address());

  ASSERT_TRUE(n3 > n1);
  ASSERT_TRUE(n2 > n3);
  ASSERT_TRUE(n2 > n1);

  ASSERT_FALSE(n1 > n3);
  ASSERT_FALSE(n3 > n2);
  ASSERT_FALSE(n1 > n2);

  data d4 = ip_address::parse_ip_value(1163468770);
  data d5 = ip_address::parse_ip_value(3628193956);
  data d6 = ip_address::parse_ip_value(static_cast<uint32_t>(4791662726));

  ASSERT_TRUE(mutable_value(n1.type(), new ip_address(1163468770)) == n1);
  ASSERT_TRUE(mutable_value(n1.type(), d4) == n1);
  ASSERT_TRUE(mutable_value(n2.type(), d5) == n2);
  ASSERT_TRUE(mutable_value(n3.type(), d6) == (n1 + n2));

  mutable_value val1 = mutable_value::parse("42.35.109.253", n1.type());
  data d7 = ip_address::parse_ip_value(706964989);
  ASSERT_EQ(706964989,
            (*reinterpret_cast<const ip_address*>(val1.ptr())).get_address());

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("-1.3.4.5", data_types[9]);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("278.1.1.1", data_types[9]);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("123412341", data_types[9]);
  }));
}

TEST_F(TypeManagerTest, SizeTypeTest) {
  data d1 = size_type::parse_bytes("3mb");
  data d2 = size_type::parse_bytes("10gb");
  data d3 = size_type::parse_bytes("3072kb");

  mutable_value n1(data_types[type_manager::get_id_from_type_name("size type")],
                   d1);
  mutable_value n2(data_types[type_manager::get_id_from_type_name("size type")],
                   d2);
  mutable_value n3(data_types[type_manager::get_id_from_type_name("size type")],
                   d3);

  ASSERT_EQ(10, n1.type().id);
  ASSERT_EQ(10, n2.type().id);
  ASSERT_EQ(10, n3.type().id);

  ASSERT_EQ(3145728,
            (*reinterpret_cast<const size_type*>(n1.ptr())).get_bytes());
  ASSERT_EQ(10737418240,
            (*reinterpret_cast<const size_type*>(n2.ptr())).get_bytes());
  ASSERT_EQ(3145728,
            (*reinterpret_cast<const size_type*>(n3.ptr())).get_bytes());

  ASSERT_TRUE(n2 > n1);
  ASSERT_TRUE(n2 > n3);
  ASSERT_TRUE(n3 == n1);

  ASSERT_FALSE(n1 > n2);
  ASSERT_FALSE(n3 > n2);
  ASSERT_FALSE(n1 > n3);

  data d4 = size_type::parse_bytes("3145728b");
  data d5 = size_type::parse_bytes("10737418240b");
  data d6 = size_type::parse_bytes("10740563968b");

  ASSERT_TRUE(mutable_value(n1.type(), new size_type(3145728)) == n1);
  ASSERT_TRUE(mutable_value(n1.type(), d4) == n1);
  ASSERT_TRUE(mutable_value(n2.type(), d5) == n2);
  ASSERT_TRUE(mutable_value(n3.type(), d6) == (n1 + n2));

  mutable_value val1 = mutable_value::parse("15pb", n1.type());
  data d7 = size_type::parse_bytes("16888498602639360b");
  ASSERT_EQ(16888498602639360,
            (*reinterpret_cast<const size_type*>(val1.ptr())).get_bytes());

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("234xb", data_types[9]);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("1234k", data_types[9]);
  }));

  ASSERT_TRUE(test::test_utils::test_fail([]() {
    mutable_value::parse("10987", data_types[9]);
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
  m4.add(predicate("a", relop_id::EQ, "4.4.4.4"));
  m5.add(predicate("b", relop_id::GT, "0.0.0.7"));
  m6.add(predicate("c", relop_id::EQ, "4kb"));

  compiled_expression cexp;
  cexp.insert(m4);
  cexp.insert(m5);
  cexp.insert(m6);

  ASSERT_STREQ("A==ip_address() or B>ip_address()C==size type()",
               cexp.to_string().c_str());

}

TEST_F(TypeManagerTest, SchemaTest) {
  schema_builder build;
  build.add_column(data_types[9], "a");
  build.add_column(
      data_types[type_manager::get_id_from_type_name("ip_address")], "b");
  build.add_column(data_types[10], "c");
  build.add_column(data_types[10], "d");
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
  ASSERT_TRUE(data_types[9] == schema_vec[1].type());
  ASSERT_FALSE(schema_vec[1].is_indexed());

  ASSERT_EQ(2, s[2].idx());
  ASSERT_EQ(12, s[2].offset());
  ASSERT_EQ("B", s[2].name());
  ASSERT_TRUE(data_types[9] == s[2].type());
  ASSERT_FALSE(s[2].is_indexed());

  ASSERT_EQ(3, s[3].idx());
  ASSERT_EQ(16, s[3].offset());
  ASSERT_EQ("C", s[3].name());
  ASSERT_TRUE(data_types[10] == s[3].type());
  ASSERT_FALSE(s[3].is_indexed());

  ASSERT_EQ(4, s[4].idx());
  ASSERT_EQ(24, s[4].offset());
  ASSERT_EQ("D", s[4].name());
  ASSERT_TRUE(data_types[10] == s[4].type());
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
  ASSERT_TRUE(data_types[9] == s[1].type());
  ASSERT_FALSE(s[1].is_indexed());

  ASSERT_EQ(2, s[2].idx());
  ASSERT_EQ(12, s[2].offset());
  ASSERT_EQ("B", s[2].name());
  ASSERT_TRUE(data_types[9] == s[2].type());
  ASSERT_FALSE(s[2].is_indexed());

  ASSERT_EQ(3, s[3].idx());
  ASSERT_EQ(16, s[3].offset());
  ASSERT_EQ("C", s[3].name());
  ASSERT_TRUE(data_types[10] == s[3].type());
  ASSERT_FALSE(s[3].is_indexed());

  ASSERT_EQ(4, s[4].idx());
  ASSERT_EQ(24, s[4].offset());
  ASSERT_EQ("D", s[4].name());
  ASSERT_TRUE(data_types[10] == s[4].type());
  ASSERT_FALSE(s[4].is_indexed());
}

TEST_F(TypeManagerTest, DataTypesGetterTest) {
  data_type t1 = data_types[9];
  data_type t2 = data_types[10];

  ASSERT_TRUE(
      data_types[type_manager::get_id_from_type_name("ip_address")] == t1);
  ASSERT_TRUE(
      data_types[type_manager::get_id_from_type_name("size type")] == t2);
  ASSERT_FALSE(t1 == t2);

  ASSERT_TRUE(t1.to_string() == "ip_address");
  ASSERT_TRUE(t2.to_string() == "size type");
}

TEST_F(TypeManagerTest, SerializeTest) {
  std::ofstream outfile("/tmp/test.txt", std::ofstream::binary);
  int* int_ptr = new int(390);
  void* void_ptr = (void*) int_ptr;
  const void* const_ptr = const_cast<const void*>(void_ptr);
  //std::cout << "Const void_ptr: " << *const_ptr << std::endl;

  data d1 = data(const_ptr, 4);
  data d11 = ip_address::parse_ip("69.89.31.226");

  ASSERT_EQ(390, d1.as<int>());
  SERIALIZERS[4](outfile, d1);
  SERIALIZERS[9](outfile, d11);
  outfile.close();
  //data d2 = DESERIALIZERS[4](infile);
  //int* first = (int*) d2.ptr;
  std::ifstream infile("/tmp/test.txt", std::ifstream::binary);
  char* buffer = new char[4];
  infile.read(buffer, 4);
  int* value_ptr = (int*) buffer;
  int value = *value_ptr;

  infile.read(buffer, 4);
  uint32_t* ip_ptr = (uint32_t*) buffer;
  uint32_t ip_value = *ip_ptr;
  ASSERT_EQ(1163468770, ip_value);

  ASSERT_EQ(390, value);
  delete[] buffer;
  delete int_ptr;
  infile.close();
}

TEST_F(TypeManagerTest, DeserializeTest) {
  std::ofstream outfile("/tmp/test1.txt", std::ofstream::binary);

  int* int_ptr = new int(-490);
  void* void_ptr = (void*) int_ptr;
  const void* const_ptr = const_cast<const void*>(void_ptr);

  data d1 = data(const_ptr, 4);
  data d11 = ip_address::parse_ip("69.89.31.226");

  ASSERT_EQ(-490, d1.as<int>());

  SERIALIZERS[4](outfile, d1);
  SERIALIZERS[9](outfile, d11);
  outfile.close();

  std::ifstream infile("/tmp/test1.txt", std::ifstream::binary);
  const void* new_alloc = (const void*) new int;
  data d3 = data(new_alloc, 4);
  DESERIALIZERS[4](infile, d3);
  ASSERT_EQ(-490, d3.as<int>());

  const void* new_ip = (const void*) new ip_address;
  data d13 = data(new_ip, 4);
  DESERIALIZERS[9](infile, d13);
  ASSERT_EQ(1163468770,
            (*reinterpret_cast<const ip_address*>(d13.ptr)).get_address());

  infile.close();
  delete int_ptr;
  //delete new_alloc;
}

#endif /* TEST_TYPE_MANAGER_TEST_H_ */

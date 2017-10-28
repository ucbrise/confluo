#ifndef TEST_TYPE_MANAGER_TEST_H_
#define TEST_TYPE_MANAGER_TEST_H_

#include "data_types.h"
#include "ip_address.h"
#include "size_type.h"
#include "dialog_table.h"
#include "gtest/gtest.h"
#include "type_manager.h"

#include <regex>

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::dialog;

//std::vector<data_type> dialog::type_manager::data_types;
//std::atomic<uint16_t> dialog::type_manager::id;

type_operators type_def(sizeof(uint32_t), 
            get_relops(), get_unaryops(),
            get_binaryops(), get_keyops(),
            &limits::int_min, &limits::int_max, &limits::int_one,
            &limits::int_zero, &ip_address::to_string, 
            &ip_address::parse_ip);

type_operators size_type_ops(sizeof(uint64_t),
        get_reops(), get_unarops(), get_binarops(), get_keops(),
        &limits::long_long_min, 
        &limits::long_long_max,
        &limits::long_long_one, &limits::long_long_zero,
        &size_type::to_string, &size_type::parse_bytes);

class TypeManagerTest : public testing::Test {
  public:
    static std::vector<column_t> schema() {
        //type_manager::register_primitives();
        type_manager::register_type(type_def);
        type_manager::register_type(size_type_ops);

        schema_builder builder;
        builder.add_column(data_types[9], "a");
        builder.add_column(data_types[9], "b");
        builder.add_column(data_types[
                type_manager::get_id_from_type_name("size type")], "c");
        
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

    ASSERT_STREQ("ip_address", 
            data_types[9].to_string().c_str());
    ASSERT_EQ(9, dialog::type_manager::get_id_from_type_name("ip_address"));
    
    ASSERT_STREQ("size type", 
            data_types[10].to_string().c_str());
    ASSERT_EQ(10, dialog::type_manager::get_id_from_type_name("size type"));

}

TEST_F(TypeManagerTest, FilterTest) {
    dialog_table dtable("my_table", s, "/tmp", storage::IN_MEMORY, 
            MGMT_POOL);
    dtable.add_index("a");
    dtable.add_index("b");
    dtable.add_index("c");

    dtable.append(record(ip_address(52), ip_address(42), 
                size_type::from_string("1024b")));
    dtable.append(record(ip_address(50), ip_address(300),
                size_type::from_string("1kb")));

    size_t i = 0;
    for (auto r = dtable.execute_filter("a > 0.0.0.3"); r.has_more(); ++r) {
        //ASSERT_TRUE(r.get().at(1).value().to_data().as<ip_address>().
        //        get_address() > 33);
        i++;
    }
    ASSERT_EQ(2, i);
    
    i = 0;
    for (auto r = dtable.execute_filter("c == 1kb"); r.has_more(); ++r) {
        i++;
    }

    ASSERT_EQ(2, i);
}

TEST_F(TypeManagerTest, IPAddressTest) {
    data d1 = ip_address::parse_ip("69.89.31.226");
    data d2 = ip_address::parse_ip("216.65.216.164");
    data d3 = ip_address::parse_ip("172.16.254.1");
    
    mutable_value n1(data_types[
            type_manager::get_id_from_type_name("ip_address")], d1);
    mutable_value n2(data_types[
            type_manager::get_id_from_type_name("ip_address")], d2);
    mutable_value n3(data_types[
            type_manager::get_id_from_type_name("ip_address")], d3);

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
    ASSERT_TRUE(mutable_value(n3.type(), d6) == 
            (n1 + n2));

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
    
    mutable_value n1(data_types[
            type_manager::get_id_from_type_name("size type")], d1);
    mutable_value n2(data_types[
            type_manager::get_id_from_type_name("size type")], d2);
    mutable_value n3(data_types[
            type_manager::get_id_from_type_name("size type")], d3);

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
    ASSERT_TRUE(mutable_value(n3.type(), d6) == 
            (n1 + n2));

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


#endif /* TEST_TYPE_MANAGER_TEST_H_ */

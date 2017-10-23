#ifndef TEST_TYPE_MANAGER_TEST_H_
#define TEST_TYPE_MANAGER_TEST_H_

#include "data_types.h"
#include "type_manager.h"
#include "ip_address.h"
#include "dialog_table.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::dialog;

std::vector<data_type> dialog::type_manager::data_types;
std::atomic<uint16_t> dialog::type_manager::id;

type_definition type_def(sizeof(ip_address), 
            get_relops(), get_unaryops(),
            get_binaryops(), get_keyops(),
            &limits::int_min, &limits::int_max, &limits::int_one,
            &limits::int_zero, &ip_address::to_string);

class TypeManagerTest : public testing::Test {
  public:
    static std::vector<column_t> schema() {
        type_manager::register_primitives();
        type_manager::register_type(type_def);

        schema_builder builder;
        builder.add_column(dialog::type_manager::data_types[9], "a");
        builder.add_column(dialog::type_manager::data_types[9], "b");
        
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
    }__attribute__((packed));

    static rec r;

    static void* record(ip_address a, ip_address b) {
        int64_t ts = utils::time_utils::cur_ns();
        r = {ts, a, b};
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
            dialog::type_manager::data_types[9].to_string().c_str());
    ASSERT_EQ(9, dialog::type_manager::get_id_from_type_name("ip_address"));
}

/*TEST_F(TypeManagerTest, FilterTest) {
    dialog_table dtable("my_table", s, "/tmp", storage::IN_MEMORY, 
            MGMT_POOL);
    dtable.append(record(ip_address(32), ip_address(42)));
    dtable.append(record(ip_address(50), ip_address(300)));

    size_t i = 0;
    for (auto r = dtable.execute_filter("a > 33"); r.has_more(); ++r) {
        ASSERT_TRUE(r.get().at(1).value().to_data().as<ip_address>().
                get_address() > 33);
        i++;
    }
    //ASSERT_EQ(1, 1);
}*/

#endif /* TEST_TYPE_MANAGER_TEST_H_ */

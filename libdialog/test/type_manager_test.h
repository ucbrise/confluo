#ifndef TEST_TYPE_MANAGER_TEST_H_
#define TEST_TYPE_MANAGER_TEST_H_

#include "data_types.h"
#include "type_manager.h"
#include "ip_address.h"
#include "gtest/gtest.h"

using namespace ::dialog;

class TypeManagerTest : public testing::Test {
};

std::vector<data_type> dialog::type_manager::data_types;
std::atomic<std::uint16_t> dialog::type_manager::id;

TEST_F(TypeManagerTest, RegisterTest) {
    type_manager::register_type(sizeof(ip_address), 
            get_relops(), get_unaryops(),
            get_binaryops(), get_keyops(),
            &limits::int_min, &limits::int_max, &limits::int_one,
            &limits::int_zero);
    int* limit_reference = (int*) (ONE[9]);
    int limit = *limit_reference;
    ASSERT_EQ(1, limit);
}

#endif /* TEST_TYPE_MANAGER_TEST_H_ */

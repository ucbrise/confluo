#include "dialog_table.h"
#include "gtest/gtest.h"

#define MAX_RECORDS 2560U
#define DATA_SIZE   64U

using namespace ::dialog;

class DiaLogTableTest : public testing::Test {
 public:
  static void gendata(unsigned char* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  template<typename dtable_t>
  void test_append_and_get(dtable_t& dtable) {
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      DiaLogTableTest::gendata(data_, DATA_SIZE, i);
      dtable.append(data_, DATA_SIZE);
    }

    unsigned char ret[DATA_SIZE];
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      bool success = dtable.get(i * DATA_SIZE, ret, DATA_SIZE);
      ASSERT_TRUE(success);
      uint8_t expected = i % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ret[j], expected);
      }
    }
  }
 protected:
  uint8_t data_[DATA_SIZE];
};

TEST_F(DiaLogTableTest, AppendAndGetTest1) {
  dialog_table<storage::in_memory> dtable(
      schema_builder().add_column(string_type(DATA_SIZE), "msg"));
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, AppendAndGetTest2) {
  dialog_table<storage::durable> dtable(
      schema_builder().add_column(string_type(DATA_SIZE), "msg"), "/tmp");
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, AppendAndGetTest3) {
  dialog_table<storage::durable_relaxed> dtable(
      schema_builder().add_column(string_type(DATA_SIZE), "msg"), "/tmp");
  test_append_and_get(dtable);
}

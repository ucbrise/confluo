#ifndef TEST_DIALOG_TABLE_TEST_H_
#define TEST_DIALOG_TABLE_TEST_H_

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
    std::vector<uint64_t> offsets;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      DiaLogTableTest::gendata(data_, DATA_SIZE, i);
      uint64_t offset = dtable.append(data_, DATA_SIZE);
      offsets.push_back(offset);
    }

    record_t r;
    for (uint64_t i = 0; i < MAX_RECORDS; i++) {
      bool success = dtable.read(offsets[i], r, DATA_SIZE);
      const uint8_t* ret = reinterpret_cast<const uint8_t*>(r.data());
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
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg"));
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, AppendAndGetTest2) {
  dialog_table<storage::durable> dtable(
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg"), "/tmp");
  test_append_and_get(dtable);
}

TEST_F(DiaLogTableTest, AppendAndGetTest3) {
  dialog_table<storage::durable_relaxed> dtable(
      schema_builder().add_column(STRING_TYPE(DATA_SIZE), "msg"), "/tmp");
  test_append_and_get(dtable);
}

#endif /* TEST_DIALOG_TABLE_TEST_H_ */

#include "log_store.h"

#include "gtest/gtest.h"

#define MAX_IDS   2560U
#define DATA_SIZE 64U

using namespace ::dialog::append_only;

class AppendOnlyLogStoreTest : public testing::Test {
 public:
  static void gendata(unsigned char* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  template<typename ls_t>
  void test_append_and_get(ls_t& ls) {
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      AppendOnlyLogStoreTest::gendata(data_, DATA_SIZE, i);
      ls.append(data_, DATA_SIZE);
    }

    unsigned char ret[DATA_SIZE];
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      bool success = ls.get(i * DATA_SIZE, ret, DATA_SIZE);
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

TEST_F(AppendOnlyLogStoreTest, AppendAndGetTest1) {
  log_store<in_memory, read_stalled> ls;
  test_append_and_get(ls);
}

TEST_F(AppendOnlyLogStoreTest, AppendAndGetTest2) {
  log_store<in_memory, write_stalled> ls;
  test_append_and_get(ls);
}

TEST_F(AppendOnlyLogStoreTest, AppendAndGetTest3) {
  log_store<durable, read_stalled> ls("/tmp");
  test_append_and_get(ls);
}

TEST_F(AppendOnlyLogStoreTest, AppendAndGetTest4) {
  log_store<durable, write_stalled> ls("/tmp");
  test_append_and_get(ls);
}

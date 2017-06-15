#include "dialog_store.h"
#include "gtest/gtest.h"

#define MAX_IDS   2560U
#define DATA_SIZE 64U

using namespace ::dialog;

class LogStoreTest : public testing::Test {
 public:
  static void gendata(unsigned char* buf, size_t len, uint64_t val) {
    uint8_t val_uint8 = (uint8_t) (val % 256);
    for (uint32_t i = 0; i < len; i++)
      buf[i] = val_uint8;
  }

  template<typename ls_t>
  void test_append_and_get(ls_t& ls) {
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      LogStoreTest::gendata(data_, DATA_SIZE, i);
      ls.append(data_, DATA_SIZE);
    }

    unsigned char ret[DATA_SIZE];
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      size_t length;
      bool success = ls.get(i, ret, length);
      ASSERT_TRUE(success);
      ASSERT_EQ(length, DATA_SIZE);
      uint8_t expected = i % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ret[j], expected);
      }
    }
  }

  template<typename ls_t>
  void test_append_update_get(ls_t& ls) {
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      LogStoreTest::gendata(data_, DATA_SIZE, i);
      ls.append(data_, DATA_SIZE);
    }

    for (uint64_t i = 0; i < MAX_IDS; i++) {
      LogStoreTest::gendata(data_, DATA_SIZE, i + 1);
      bool success = ls.update(i, data_, DATA_SIZE);
      ASSERT_TRUE(success);
    }

    unsigned char ret[DATA_SIZE];
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      size_t length;
      bool success = ls.get(i, ret, length);
      ASSERT_TRUE(success);
      ASSERT_EQ(length, DATA_SIZE);
      uint8_t expected = (i + 1) % 256;
      for (uint32_t j = 0; j < DATA_SIZE; j++) {
        ASSERT_EQ(ret[j], expected);
      }
    }
  }

  template<typename ls_t>
  void test_append_invalidate_get(ls_t& ls) {
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      LogStoreTest::gendata(data_, DATA_SIZE, i);
      ls.append(data_, DATA_SIZE);
    }

    for (uint64_t i = 0; i < MAX_IDS; i++) {
      LogStoreTest::gendata(data_, DATA_SIZE, i + 1);
      bool success = ls.invalidate(i);
      ASSERT_TRUE(success);
    }

    unsigned char ret[DATA_SIZE];
    for (uint64_t i = 0; i < MAX_IDS; i++) {
      size_t length;
      bool success = ls.get(i, ret, length);
      ASSERT_FALSE(success);
    }
  }

 protected:
  uint8_t data_[DATA_SIZE];
};

TEST_F(LogStoreTest, AppendAndGetTest1) {
  dialog::dialog_store<in_memory, read_stalled> ls;
  test_append_and_get(ls);
}

TEST_F(LogStoreTest, AppendAndGetTest2) {
  dialog::dialog_store<in_memory, write_stalled> ls;
  test_append_and_get(ls);
}

TEST_F(LogStoreTest, AppendAndGetTest3) {
  dialog::dialog_store<durable, read_stalled> ls("/tmp");
  test_append_and_get(ls);
}

TEST_F(LogStoreTest, AppendAndGetTest4) {
  dialog::dialog_store<durable, write_stalled> ls("/tmp");
  test_append_and_get(ls);
}

TEST_F(LogStoreTest, AppendUpdateGetTest1) {
  dialog::dialog_store<in_memory, read_stalled> ls;
  test_append_update_get(ls);
}

TEST_F(LogStoreTest, AppendUpdateGetTest2) {
  dialog::dialog_store<in_memory, write_stalled> ls;
  test_append_update_get(ls);
}

TEST_F(LogStoreTest, AppendUpdateGetTest3) {
  dialog::dialog_store<durable, read_stalled> ls("/tmp");
  test_append_update_get(ls);
}

TEST_F(LogStoreTest, AppendUpdateGetTest4) {
  dialog::dialog_store<durable, write_stalled> ls("/tmp");
  test_append_update_get(ls);
}

TEST_F(LogStoreTest, AppendDeleteGetTest1) {
  dialog::dialog_store<in_memory, read_stalled> ls;
  test_append_invalidate_get(ls);
}

TEST_F(LogStoreTest, AppendDeleteGetTest2) {
  dialog::dialog_store<in_memory, write_stalled> ls;
  test_append_invalidate_get(ls);
}

TEST_F(LogStoreTest, AppendDeleteGetTest3) {
  dialog::dialog_store<durable, read_stalled> ls("/tmp");
  test_append_invalidate_get(ls);
}

TEST_F(LogStoreTest, AppendDeleteGetTest4) {
  dialog::dialog_store<durable, write_stalled> ls("/tmp");
  test_append_invalidate_get(ls);
}

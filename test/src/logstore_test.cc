#include "logstore.h"

#include "gtest/gtest.h"

#define kLogStoreSize 4194304
#define kMaxKeys 1000
class LogStoreTest : public testing::Test {
 public:
  std::string to_string(uint64_t i) {
    char buf[6];
    sprintf(buf, "%05d", i);
    return std::string(buf);
  }
};

TEST_F(LogStoreTest, AppendAndGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.append(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    ls.get(value, i);
    ASSERT_EQ(std::string(value), to_string(i));
  }
}

TEST_F(LogStoreTest, AppendAndSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.append(i, "|" + to_string(i) + "|");
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    ls.search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 1);
    ASSERT_TRUE(results.find(i) != results.end());
  }
}

TEST_F(LogStoreTest, AppendDeleteGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.append(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    int ret = ls.get(value, i);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(std::string(value), to_string(i));

    bool success = ls.delete_record(i);
    ASSERT_TRUE(success);

    ret = ls.get(value, i);
    ASSERT_EQ(ret, -1);
  }
}

TEST_F(LogStoreTest, AppendDeleteSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.append(i, "|" + to_string(i) + "|");
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    ls.search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 1);
    ASSERT_TRUE(results.find(i) != results.end());
    results.clear();

    bool success = ls.delete_record(i);
    ASSERT_TRUE(success);

    ls.search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 0);
  }
}

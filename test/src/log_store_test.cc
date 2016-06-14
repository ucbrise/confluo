#include "log_store.h"
#include "gtest/gtest.h"

#define kLogStoreSize 4194304
#define kMaxKeys 1000
class LogStoreTest : public testing::Test {
 public:
  std::string to_string(uint64_t i) {
    char buf[5];
    sprintf(buf, "%04d", i);
    return std::string(buf);
  }
};

TEST_F(LogStoreTest, AppendAndGetTest) {
  succinct::LogStore<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.Append(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    ls.Get(value, i);
    ASSERT_EQ(std::string(value), to_string(i));
  }
}

TEST_F(LogStoreTest, AppendAndSearchTest) {
  succinct::LogStore<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.Append(i, "|" + to_string(i) + "|");
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    ls.Search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 1);
    ASSERT_TRUE(results.find(i) != results.end());
  }
}

TEST_F(LogStoreTest, AppendDeleteGetTest) {
  succinct::LogStore<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.Append(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    int ret = ls.Get(value, i);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(std::string(value), to_string(i));

    bool success = ls.Delete(i);
    ASSERT_TRUE(success);

    ret = ls.Get(value, i);
    ASSERT_EQ(ret, -1);
  }
}

TEST_F(LogStoreTest, AppendDeleteSearchTest) {
  succinct::LogStore<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.Append(i, "|" + to_string(i) + "|");
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    ls.Search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 1);
    ASSERT_TRUE(results.find(i) != results.end());
    results.clear();

    bool success = ls.Delete(i);
    ASSERT_TRUE(success);

    ls.Search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 0);
  }
}

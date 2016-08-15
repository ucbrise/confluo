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

TEST_F(LogStoreTest, InsertAndGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    ls.get(value, i);
    ASSERT_EQ(std::string(value), to_string(i));
  }
}

TEST_F(LogStoreTest, InsertAndSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, "|" + to_string(i) + "|");
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    ls.search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 1);
    ASSERT_TRUE(results.find(i) != results.end());
  }
}

TEST_F(LogStoreTest, InsertDeleteGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    bool exists = ls.get(value, i);
    ASSERT_TRUE(exists);
    ASSERT_EQ(std::string(value), to_string(i));

    bool deleted = ls.delete_record(i);
    ASSERT_TRUE(deleted);

    exists = ls.get(value, i);
    ASSERT_FALSE(exists);
  }
}

TEST_F(LogStoreTest, InsertDeleteSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, "|" + to_string(i) + "|");
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

TEST_F(LogStoreTest, UDefInsertAndGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize, slog::udef_kvmap> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    ls.get(value, i);
    ASSERT_EQ(std::string(value), to_string(i));
  }
}

TEST_F(LogStoreTest, UDefInsertAndSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize, slog::udef_kvmap> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, "|" + to_string(i) + "|");
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    std::set<int64_t> results;
    ls.search(results, "|" + to_string(i) + "|");
    ASSERT_EQ(results.size(), 1);
    ASSERT_TRUE(results.find(i) != results.end());
  }
}

TEST_F(LogStoreTest, UDefInsertDeleteGetTest) {
  slog::log_store<kMaxKeys, kLogStoreSize, slog::udef_kvmap> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, to_string(i));
  }

  for (uint64_t i = 0; i < kMaxKeys; i++) {
    char value[5];
    bool exists = ls.get(value, i);
    ASSERT_TRUE(exists);
    ASSERT_EQ(std::string(value), to_string(i));

    bool deleted = ls.delete_record(i);
    ASSERT_TRUE(deleted);

    exists = ls.get(value, i);
    ASSERT_FALSE(exists);
  }
}

TEST_F(LogStoreTest, UDefInsertDeleteSearchTest) {
  slog::log_store<kMaxKeys, kLogStoreSize, slog::udef_kvmap> ls;
  for (uint64_t i = 0; i < kMaxKeys; i++) {
    ls.insert(i, "|" + to_string(i) + "|");
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

#define STL_LOCKS
#include "splitordered/hashtable.h"
#include "gtest/gtest.h"

#include <thread>
#include <vector>
#include <algorithm>

class SplitOrderedTest : public testing::Test {
 public:
  const uint64_t kMaxSize = 100 * 1024ULL;  // 100K entries
  const uint64_t kNumThreads = 10;  // 10 threads
};

TEST_F(SplitOrderedTest, InsertAndGetTest) {
  splitordered::hash_table<uint64_t> table;
  for (uint64_t i = 0; i < kMaxSize; i++) {
    bool success = table.insert(i, i);
    ASSERT_TRUE(success);
  }

  for (uint64_t i = 0; i < kMaxSize; i++) {
    bool success = table.insert(i, i);
    ASSERT_FALSE(success);
  }

  for (uint64_t i = 0; i < kMaxSize; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_TRUE(success);
    ASSERT_EQ(ret, i);
  }

  for (uint64_t i = kMaxSize; i < 2 * kMaxSize; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_FALSE(success);
  }
}

TEST_F(SplitOrderedTest, ConcurrentInsertAndGetTest) {
  splitordered::hash_table<uint64_t> table;
  std::vector<std::thread> threads;
  for (uint64_t t = 0; t < kNumThreads; t++) {
    threads.push_back(std::thread([this, t, &table]()
    {
      for (uint64_t i = 0; i < kMaxSize; i++) {
        bool success = table.insert(t * kMaxSize + i, i);
        ASSERT_TRUE(success);
      }
    }));
  }

  std::for_each(threads.begin(), threads.end(), [](std::thread &t)
  {
    t.join();
  });

  for (uint64_t i = 0; i < kMaxSize * kNumThreads; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_TRUE(success);
    ASSERT_EQ(ret, i % kMaxSize);
  }

  uint64_t limit = kMaxSize * kNumThreads;
  for (uint64_t i = limit; i < 2 * limit; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_FALSE(success);
  }
}

TEST_F(SplitOrderedTest, ConcurrentInsertAndConcurrentGetTest) {
  splitordered::hash_table<uint64_t> table;
  std::vector<std::thread> threads;
  for (uint64_t t = 0; t < kNumThreads; t++) {
    threads.push_back(std::thread([this, t, &table]()
    {
      for (uint64_t i = 0; i < kMaxSize; i++) {
        bool success = table.insert(t * kMaxSize + i, i);
        ASSERT_TRUE(success);
      }

      for (uint64_t i = 0; i < kMaxSize; i++) {
        uint64_t ret;
        bool success = table.get(t * kMaxSize + i, &ret);
        ASSERT_TRUE(success);
        ASSERT_EQ(ret, i);
      }
    }));
  }

  std::for_each(threads.begin(), threads.end(), [](std::thread &t)
  {
    t.join();
  });
}

TEST_F(SplitOrderedTest, UpsertAndGetTest) {
  splitordered::hash_table<uint64_t> table;
  for (uint64_t i = 0; i < kMaxSize; i++) {
    table.upsert(i, i);
  }

  for (uint64_t i = 0; i < kMaxSize; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_TRUE(success);
    ASSERT_EQ(ret, i);
  }

  for (uint64_t i = kMaxSize; i < 2 * kMaxSize; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_FALSE(success);
  }
}

TEST_F(SplitOrderedTest, ConcurrentUpsertAndGetTest) {
  splitordered::hash_table<uint64_t> table;
  std::vector<std::thread> threads;
  for (uint64_t t = 0; t < kNumThreads; t++) {
    threads.push_back(std::thread([this, t, &table]()
    {
      for (uint64_t i = 0; i < kMaxSize; i++) {
        table.upsert(i, t);
      }
    }));
  }

  std::for_each(threads.begin(), threads.end(), [](std::thread &t)
  {
    t.join();
  });

  for (uint64_t i = 0; i < kMaxSize; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_TRUE(success);
    ASSERT_EQ(kNumThreads - 1, ret);
  }

  for (uint64_t i = kMaxSize; i < 2 * kMaxSize; i++) {
    uint64_t ret;
    bool success = table.get(i, &ret);
    ASSERT_FALSE(success);
  }
}

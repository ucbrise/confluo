#define STL_LOCKS
#include "splitordered/hashtable.h"
#include "gtest/gtest.h"

class SplitOrderedTest : public testing::Test {
 public:
  const uint64_t kMaxSize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(SplitOrderedTest, PutAndGetTest) {
  splitordered::hash_table<uint64_t> table;
  for (uint64_t i = 0; i < kMaxSize; i++) {
    table.put(i, i);
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

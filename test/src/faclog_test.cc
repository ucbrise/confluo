#define STL_LOCKS
#include "faclog.h"
#include "gtest/gtest.h"

class FACLogTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(FACLogTest, FACLogBaseBaseTest) {
  slog::__faclog_base<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.set(i, i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.get(i), i);
  }
}

TEST_F(FACLogTest, FACLogConsistentTest) {
  slog::faclog_consistent<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.push_back(i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.at(i), i);
  }
}

TEST_F(FACLogTest, FACLogRelaxedTest) {
  slog::faclog_relaxed<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.push_back(i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.at(i), i);
  }
}

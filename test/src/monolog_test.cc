#include "monolog.h"
#include "gtest/gtest.h"

class MonoLogTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(MonoLogTest, MonoLogBaseBaseTest) {
  slog::__monolog_base<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.set(i, i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.get(i), i);
  }
}

TEST_F(MonoLogTest, AtomicMonoLogBaseBaseTest) {
  slog::__atomic_monolog_base<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.store(i, i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.load(i), i);
  }
}

TEST_F(MonoLogTest, MonoLogConsistentTest) {
  slog::monolog_linearizable<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.push_back(i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.at(i), i);
  }
}

TEST_F(MonoLogTest, MonoLogRelaxedTest) {
  slog::monolog_relaxed<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.push_back(i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.at(i), i);
  }
}

#define STL_LOCKS
#include "conc_vectors.h"
#include "gtest/gtest.h"

class ConcVectorsTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
  const uint8_t kBitWidth = 20;  // 20 bits
};

TEST_F(ConcVectorsTest, LockBasedVectorTest) {
  ConcurrentVector<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.push_back(i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.at(i), i);
  }
}

TEST_F(ConcVectorsTest, LockFreeBaseTest) {
  __LockFreeBase<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.set(i, i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.get(i), i);
  }
}

TEST_F(ConcVectorsTest, LockFreeBaseAtomicTest) {
  __LockFreeBaseAtomic<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.set(i, i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.get(i), i);
  }
}

TEST_F(ConcVectorsTest, LockFreeVectorTest) {
  LockFreeAtomicList<uint64_t> array;
  for (uint64_t i = 0; i < kArraySize; i++) {
    array.push_back(i);
  }

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(array.at(i), i);
  }
}

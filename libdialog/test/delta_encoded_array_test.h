#ifndef TEST_DELTA_ENCODED_ARRAY_TEST_H_
#define TEST_DELTA_ENCODED_ARRAY_TEST_H_

#include "delta_encoded_array.h"
#include "gtest/gtest.h"

using namespace confluo;

class DeltaEncodedArrayTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(DeltaEncodedArrayTest, EliasGammaEncodedArrayTest) {

  uint64_t *array = new uint64_t[kArraySize];
  for (uint64_t i = 0; i < kArraySize; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, kArraySize);

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(i, enc_array[i]);
  }
}

#endif /* TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

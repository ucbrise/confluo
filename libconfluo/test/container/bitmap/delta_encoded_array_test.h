#ifndef CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_
#define CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_

#include "container/bitmap/delta_encoded_array.h"
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

TEST_F(DeltaEncodedArrayTest, EliasToStream) {
  uint64_t *array = new uint64_t[kArraySize];
  for (uint64_t i = 0; i < kArraySize; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, kArraySize);
  size_t size = enc_array.byte_array_length();

  char buffer[size];
  size_t byte_array_size = enc_array.to_byte_array(buffer);

  std::cout << "Size from precompute: " << size << "Size after: " <<
      byte_array_size << std::endl;

  size_t from_size = enc_array.from_byte_array(buffer);
}

#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

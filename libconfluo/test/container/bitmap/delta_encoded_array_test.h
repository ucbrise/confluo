#ifndef CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_
#define CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_

#include "container/bitmap/delta_encoded_array.h"
#include "gtest/gtest.h"

using namespace confluo;

class DeltaEncodedArrayTest : public testing::Test {
 public:
  const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(DeltaEncodedArrayTest, EliasGammaEncodedArrayOneElemTest) {
  uint64_t* array = new uint64_t[1];
  array[0] = 10;
  elias_gamma_encoded_array<uint64_t> enc_array(array, 1);
  size_t buffer_size = enc_array.storage_size() + sizeof(size_t);
}

TEST_F(DeltaEncodedArrayTest, EliasGammaEncodedArrayTest) {

  uint64_t* array = new uint64_t[kArraySize];
  for (uint64_t i = 0; i < kArraySize; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, kArraySize);

  for (uint64_t i = 0; i < kArraySize; i++) {
    ASSERT_EQ(i, enc_array[i]);
  }
}

TEST_F(DeltaEncodedArrayTest, ToFromByteArrayTest) {
  uint64_t k_array_size = 1000;

  uint64_t* array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, k_array_size);
  size_t size = enc_array.storage_size();

  uint8_t* buffer = new uint8_t[size + sizeof(size_t)];
  std::memcpy(buffer, &size, sizeof(size_t));
  size_t byte_array_size = enc_array.to_byte_array(buffer + sizeof(size_t));

  ASSERT_EQ(size, byte_array_size);

  elias_gamma_encoded_array<uint64_t> test_array;
  size_t from_size = test_array.from_byte_array(buffer + sizeof(size_t));

  for (uint64_t i = 0; i < k_array_size; i++) {
    ASSERT_EQ(array[i], test_array[i]);
  }

  delete[] buffer;
}


#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

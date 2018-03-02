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

TEST_F(DeltaEncodedArrayTest, ToFromByteArrayTest) {
  uint64_t k_array_size = 1000;

  uint64_t *array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, k_array_size);
  size_t size = enc_array.storage_size();

  uint8_t buffer[size];
  size_t byte_array_size = enc_array.to_byte_array(buffer);

  ASSERT_EQ(size, byte_array_size);

  uint64_t *t_array = new uint64_t[k_array_size];

  elias_gamma_encoded_array<uint64_t> test_array(t_array, k_array_size);
  size_t from_size = test_array.from_byte_array(buffer);

  for (uint64_t i = 0; i < k_array_size; i++) {
    ASSERT_EQ(array[i], test_array[i]);
  }
}

TEST_F(DeltaEncodedArrayTest, DecodeFullTest) {
  uint64_t k_array_size = 1000;

  uint64_t *array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, k_array_size);
  uint64_t buffer[k_array_size];
  enc_array.decode_full(buffer, k_array_size);

  for (uint64_t i = 0; i < k_array_size; i++) {
    ASSERT_EQ(array[i], buffer[i]);
  }
}

TEST_F(DeltaEncodedArrayTest, DecodePartialTest) {
  uint64_t k_array_size = 1000;

  uint64_t *array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, k_array_size);

  size_t src_index = 250;
  size_t buffer_size = 400;

  uint64_t buffer[buffer_size];
  enc_array.decode_partial(buffer, src_index, buffer_size);

  for (uint64_t i = 0; i < buffer_size; i++) {
    ASSERT_EQ(array[i + src_index], buffer[i]);
  }
}

TEST_F(DeltaEncodedArrayTest, DecodePtrIndexTest) {
  uint64_t k_array_size = 1000;

  uint64_t *array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, k_array_size);

  size_t src_index = 250;

  uint64_t buffer[k_array_size - src_index];
  enc_array.decode_ptr_index(buffer, src_index, k_array_size);

  for (uint64_t i = 0; i < k_array_size - src_index; i++) {
    ASSERT_EQ(array[i + src_index], buffer[i]);
  }
}

TEST_F(DeltaEncodedArrayTest, DecodeIndexTest) {
  uint64_t k_array_size = 1000;

  uint64_t *array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i;
  }

  elias_gamma_encoded_array<uint64_t> enc_array(array, k_array_size);

  size_t src_index = 250;
  size_t decoded_val = enc_array.decode_index(src_index);

  ASSERT_EQ(array[src_index], decoded_val);
}



#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

#ifndef CONFLUO_TEST_DELTA_ENCODE_TEST_H_
#define CONFLUO_TEST_DELTA_ENCODE_TEST_H_

#include "compression/delta_encoder.h"
#include "compression/delta_decoder.h"
#include "container/bitmap/delta_encoded_array.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <ctime>

using namespace confluo;
using namespace confluo::compression;

class DeltaEncodeTest : public testing::Test {
 public:
    const uint64_t kArraySize = (1024ULL * 1024ULL);  // 1 KBytes
};

TEST_F(DeltaEncodeTest, DecodeFullTest) {
  // initialized array size
  size_t k_array_size = 1024;

  // initializes starting array with monotonically increasing elements with constant increments 
  uint64_t* array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    array[i] = i * 16;
  }

  // initializes destination array
  uint64_t *dest_buffer = new uint64_t[k_array_size];
  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);
  // decodes the buffer into destination array
  delta_decoder::decode<uint64_t>(encoded_buffer.get(), dest_buffer);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < k_array_size; i++) {
    ASSERT_EQ(array[i], dest_buffer[i]);
  }

  delete[] array;
  delete[] dest_buffer;
}

TEST_F(DeltaEncodeTest, DecodePartialTest) {
  // initialized array size
  size_t k_array_size = 1024;

  // initializes starting array with monotonically increasing elements with constant increments 
  uint64_t* array = new uint64_t[k_array_size];
  for (size_t i = 0; i < k_array_size; i++) {
    array[i] = i * 16;
  }

  size_t src_index = 250;
  size_t buffer_size = 600;

  // initializes destination array that is smaller than starting array
  uint64_t *dest_buffer = new uint64_t[buffer_size];
  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);
  // partially decodes (buffer_size) the buffer into destination array from the index
  delta_decoder::decode(encoded_buffer.get(), dest_buffer, src_index, buffer_size);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < buffer_size; i++) {
    ASSERT_EQ(array[i + src_index], dest_buffer[i]);
  }

  delete[] array;
  delete[] dest_buffer;
}

TEST_F(DeltaEncodeTest, DecodePtrIndexTest) {
  // initialized array size
  size_t k_array_size = 1024;

  // initializes starting array with monotonically increasing elements with constant increments 
  uint64_t *array = new uint64_t[k_array_size];
  for (size_t i = 0; i < k_array_size; i++) {
    array[i] = i * 16;
  }
  
  size_t src_index = 250;
  size_t buffer_size = k_array_size - src_index;

  // initializes destination array that is smaller than starting array
  uint64_t *dest_buffer = new uint64_t[buffer_size];
  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);
  // decodes the buffer into destination array from the index
  delta_decoder::decode(encoded_buffer.get(), dest_buffer, src_index);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < buffer_size; i++) {
    ASSERT_EQ(array[i + src_index], dest_buffer[i]);
  }

  delete[] array;
  delete[] dest_buffer;
}

TEST_F(DeltaEncodeTest, DecodeIndexTest) {
  // initialized array size
  size_t k_array_size = 1024;

  // initializes starting array with monotonically increasing elements with constant increments 
  uint64_t* array = new uint64_t[k_array_size];
  for (size_t i = 0; i < k_array_size; i++) {
    array[i] = i * 16;
  }

  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);

  // checks to see if starting elements and decoded elements are the same
  for (size_t i = 0; i < k_array_size; i++) {
    ASSERT_EQ(array[i], delta_decoder::decode<uint64_t>(encoded_buffer.get(), i));
  }

  delete[] array;
}

TEST_F(DeltaEncodeTest, DecodeFullRandTest) {
  // initialized array size
  size_t k_array_size = 1024;
  // sets seed based on time
  srand((int)time(0));

  // initializes starting array with random elements 
  uint64_t* array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    int r = rand();
    array[i] = r;
  }

  // initializes destination array
  uint64_t *dest_buffer = new uint64_t[k_array_size];
  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);
  // decodes the buffer into destination array
  delta_decoder::decode<uint64_t>(encoded_buffer.get(), dest_buffer);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < k_array_size; i++) {
    ASSERT_EQ(array[i], dest_buffer[i]);
  }

  delete[] array;
  delete[] dest_buffer;
}

TEST_F(DeltaEncodeTest, DecodePartialRandTest) {
  // initialized array size
  size_t k_array_size = 1024;
  // sets seed based on time
  srand((int)time(0));

  // initializes starting array with random elements 
  uint64_t* array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    int r = rand();
    array[i] = r;
  }

  size_t src_index = 250;
  size_t buffer_size = 600;

  // initializes destination array that is smaller than starting array
  uint64_t *dest_buffer = new uint64_t[buffer_size];
  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);
  // partially decodes (buffer_size) the buffer into destination array from the index
  delta_decoder::decode(encoded_buffer.get(), dest_buffer, src_index, buffer_size);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < buffer_size; i++) {
    ASSERT_EQ(array[i + src_index], dest_buffer[i]);
  }

  delete[] array;
  delete[] dest_buffer;
}

TEST_F(DeltaEncodeTest, DecodePtrIndexRandTest) {
  // initialized array size
  size_t k_array_size = 1024;
  // sets seed based on time
  srand((int)time(0));

  // initializes starting array with random elements 
  uint64_t* array = new uint64_t[k_array_size];
  for (uint64_t i = 0; i < k_array_size; i++) {
    int r = rand();
    array[i] = r;
  }
  
  size_t src_index = 250;
  size_t buffer_size = k_array_size - src_index;

  // initializes destination array that is smaller than starting array
  uint64_t *dest_buffer = new uint64_t[buffer_size];
  // encodes the starting array into buffer
  auto encoded_buffer = delta_encoder::encode(array, k_array_size);
  // decodes the buffer into destination array from the index
  delta_decoder::decode(encoded_buffer.get(), dest_buffer, src_index);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < buffer_size; i++) {
    ASSERT_EQ(array[i + src_index], dest_buffer[i]);
  }

  delete[] array;
  delete[] dest_buffer;
}


#endif /* CONFLUO_TEST_DELTA_ENCODE_TEST_H_ */

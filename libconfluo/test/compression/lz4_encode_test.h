#ifndef CONFLUO_TEST_LZ4_ENCODE_TEST_H_
#define CONFLUO_TEST_LZ4_ENCODE_TEST_H_

#include "compression/lz4_encoder.h"
#include "compression/lz4_decoder.h"
#include "gtest/gtest.h"

using namespace confluo;
using namespace confluo::compression;

class LZ4EncodeTest : public testing::Test {
 public:
  static const size_t BYTES_PER_BLOCK = 1000;
};

size_t const LZ4EncodeTest::BYTES_PER_BLOCK;

TEST_F(LZ4EncodeTest, EncodeDecodeFullTest) {
  // initialized array size
  size_t size = 11048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  // initializes destination array of same size
  uint8_t *destination = new uint8_t[size];
  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  // decodes the buffer into destination array
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

/**
 * Tests case where block size > source buffer size
 */
TEST_F(LZ4EncodeTest, EncodeDecodeFullBigBlockTest) {
  // initialized array size
  size_t size = 2048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  // initializes destination array of same size
  uint8_t *destination = new uint8_t[size];
  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<>::encode(source, size);
  // decodes the buffer into destination array
  lz4_decoder<>::decode(encoded_buffer.get(), destination);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
  // initialized array size
  size_t size = 11048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  size_t dest_size = BYTES_PER_BLOCK * 2 + 210;
  int src_index = 7210;

  // initializes destination array of smaller size
  uint8_t *destination = new uint8_t[dest_size];
  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  // partially decodes (dest_size) the buffer into destination array from the index
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index, dest_size);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialLessThanBlockSizeTest) {
  // initialized array size
  size_t size = 11048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  size_t dest_size = 210;
  int src_index = 7210;
  // gets an upper bound on the size of the encoded buffer in bytes
  size_t encode_buffer_size = lz4_encoder<>::get_buffer_size(size);

  // initializes destination array of smaller size
  uint8_t *destination = new uint8_t[dest_size];
  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  // partially decodes (dest_size) the buffer into destination array from the index
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index, dest_size);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialLastIndexTest) {
  // initialized array size
  size_t size = 11048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  size_t dest_size = 790;
  int src_index = 9210;

  // initializes destination array of smaller size
  uint8_t *destination = new uint8_t[dest_size];
  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  // partially decodes (dest_size) the buffer into destination array from the index
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index, dest_size);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodeIndexTest) {
  // initialized array size
  size_t size = 11048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  int src_index = 7210;

  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  // decodes one byte at the index
  uint8_t decoded_value = lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), src_index);

  //checks to see if the decoded one byte is the same as the starting
  ASSERT_EQ(source[src_index], decoded_value);

  delete[] source;

}

TEST_F(LZ4EncodeTest, EncodeDecodeIdxPtrTest) {
  // initialized array size
  size_t size = 11048;
  uint8_t *source = new uint8_t[size];

  // populates starting array with indexes modded by 256
  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  int src_index = 230;
  size_t dest_size = size - src_index;

  // initializes destination array of smaller size
  uint8_t *destination = new uint8_t[dest_size];
  // encodes the starting array into buffer
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  // decodes the buffer into destination array from the index
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index);

  // checks to see if starting array and destination array have same elements
  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

#endif /* CONFLUO_TEST_LZ4_ENCODE_TEST_H_ */

#ifndef CONFLUO_TEST_LZ4_ENCODE_TEST_H_
#define CONFLUO_TEST_LZ4_ENCODE_TEST_H_

#include "container/bitmap/lz4_encoder.h"
#include "container/bitmap/lz4_decoder.h"
#include "gtest/gtest.h"

using namespace confluo;

class LZ4EncodeTest : public testing::Test {
 public:
};

TEST_F(LZ4EncodeTest, EncodeDecodeFullTest) {
  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  uint8_t destination[size];
  uint8_t encode_buffer[lz4_encoder::get_buffer_size(size, bytes_per_block)];

  size_t encode_size = lz4_encoder::encode(source, size, 
          bytes_per_block, encode_buffer);
  lz4_decoder::decode_full(encode_buffer, encode_size, size,
          bytes_per_block, destination);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }

}

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t dest_size = bytes_per_block * 2 + 210;
  int src_index = 7210;
  size_t encode_buffer_size = lz4_encoder::get_buffer_size(size, 
          bytes_per_block); 

  uint8_t destination[dest_size];
  uint8_t encode_buffer[encode_buffer_size];

  size_t encode_size = lz4_encoder::encode(source, size, 
          bytes_per_block, encode_buffer);
  lz4_decoder::decode_partial(encode_buffer, encode_size, size, bytes_per_block, destination, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

}

TEST_F(LZ4EncodeTest, EncodeDecodePartialLessThanBlockSizeTest) {
  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t dest_size = 210;
  int src_index = 7210;
  size_t encode_buffer_size = lz4_encoder::get_buffer_size(size, 
          bytes_per_block); 

  uint8_t destination[dest_size];
  uint8_t encode_buffer[encode_buffer_size];

  size_t encode_size = lz4_encoder::encode(source, size, 
          bytes_per_block, encode_buffer);
  lz4_decoder::decode_partial(encode_buffer, encode_size, size, bytes_per_block, destination, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

}

TEST_F(LZ4EncodeTest, EncodeDecodePartialLastIndexTest) {
  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t dest_size = 790;
  int src_index = 9210;
  size_t encode_buffer_size = lz4_encoder::get_buffer_size(size, 
          bytes_per_block); 

  uint8_t destination[dest_size];
  uint8_t encode_buffer[encode_buffer_size];

  size_t encode_size = lz4_encoder::encode(source, size, 
          bytes_per_block, encode_buffer);
  lz4_decoder::decode_partial(encode_buffer, encode_size, size, bytes_per_block, destination, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

}

TEST_F(LZ4EncodeTest, EncodeDecodeIndexTest) {
  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  int src_index = 7210;
  size_t encode_buffer_size = lz4_encoder::get_buffer_size(size, 
          bytes_per_block); 

  uint8_t encode_buffer[encode_buffer_size];

  size_t encode_size = lz4_encoder::encode(source, size, 
          bytes_per_block, encode_buffer);
  size_t decoded_value = lz4_decoder::decode_index(encode_buffer, encode_size, bytes_per_block, size, src_index);

  ASSERT_EQ(source[src_index], decoded_value);

}

TEST_F(LZ4EncodeTest, EncodeDecodeIdxPtrTest) {

  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  int src_index = 230;
  size_t dest_size = size - src_index;

  uint8_t destination[dest_size];
  uint8_t encode_buffer[lz4_encoder::get_buffer_size(size, 
          bytes_per_block)];

  size_t encode_size = lz4_encoder::encode(source, size, bytes_per_block,
          encode_buffer);

  lz4_decoder::decode_index_ptr(encode_buffer, encode_size, size, bytes_per_block, destination, src_index);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

}

#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

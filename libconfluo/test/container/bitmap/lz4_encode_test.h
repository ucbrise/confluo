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
  uint8_t encode_buffer[lz4_encoder::get_buffer_size(size)];

  std::vector<size_t> compressed_sizes;

  lz4_encoder::encode(source, size, bytes_per_block,
          &compressed_sizes, encode_buffer);
  lz4_decoder::decode_full(encode_buffer, &compressed_sizes, 
          bytes_per_block, destination, size);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }

  compressed_sizes.clear();
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
  size_t size = 1000;
  size_t bytes_per_block = 100;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t dest_size = bytes_per_block + 30;
  int src_index = 210;

  uint8_t destination[dest_size];
  uint8_t encode_buffer[lz4_encoder::get_buffer_size(size)];

  std::vector<size_t> compressed_sizes;

  lz4_encoder::encode(source, size, bytes_per_block,
          &compressed_sizes, encode_buffer);
  lz4_decoder::decode_partial(encode_buffer, &compressed_sizes, bytes_per_block, destination, bytes_per_block, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  compressed_sizes.clear();

}

TEST_F(LZ4EncodeTest, EncodeDecodeIndexTest) {

  size_t size = 10000;
  size_t bytes_per_block = 1000;
  uint8_t source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  int src_index = 230;
  size_t dest_size = size - src_index;

  uint8_t destination[dest_size];
  uint8_t encode_buffer[lz4_encoder::get_buffer_size(size)];

  std::vector<size_t> compressed_sizes;

  lz4_encoder::encode(source, size, bytes_per_block,
          &compressed_sizes, encode_buffer);

  lz4_decoder::decode_index(encode_buffer, &compressed_sizes, bytes_per_block, destination, bytes_per_block, src_index);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  compressed_sizes.clear();
 
}

#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

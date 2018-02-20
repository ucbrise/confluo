#ifndef CONFLUO_TEST_LZ4_ENCODE_TEST_H_
#define CONFLUO_TEST_LZ4_ENCODE_TEST_H_

#include "container/bitmap/lz4_encode.h"
#include "container/bitmap/lz4_decode.h"
#include "gtest/gtest.h"

using namespace confluo;

class LZ4EncodeTest : public testing::Test {
 public:
};

TEST_F(LZ4EncodeTest, EncodeDecodeFullTest) {
  lz4_encode encoder;
  lz4_decode decoder;

  size_t size = 10000;
  char source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t buffer_size = encoder.get_buffer_size(size);
  char buffer[buffer_size];

  size_t compressed_size = encoder.encode(buffer, source, size);

  char destination[size];
  size_t decompressed_size = decoder.decode_full(destination, buffer, 
          compressed_size, size);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
  lz4_encode encoder;
  lz4_decode decoder;

  size_t size = 10000;
  char source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t buffer_size = encoder.get_buffer_size(size);
  char buffer[buffer_size];

  size_t compress_size_one = encoder.encode(buffer, source, size / 2);
  size_t compress_size_two = encoder.encode(buffer + compress_size_one,
          source + size / 2, size / 2);


  char destination_partial[compress_size_two];
  int block_size = 16;
  int position = size / 2;
  int decomp_size = decoder.decode_partial(destination_partial, buffer,
          compress_size_two, block_size, position);
  for (int i = 0; i < block_size; i++) {
    ASSERT_EQ(source[i], destination_partial[i]);
  }
}

TEST_F(LZ4EncodeTest, EncodeDecodeIndexTest) {
  lz4_encode encoder;
  lz4_decode decoder;

  size_t size = 10000;
  char source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  size_t buffer_size = encoder.get_buffer_size(size);
  char buffer[buffer_size];

  size_t compress_size_one = encoder.encode(buffer, source, size / 2);
  size_t compress_size_two = encoder.encode(buffer + compress_size_one,
          source + size / 2, size / 2);


  char destination_partial[compress_size_two];
  int block_size = 16;
  int position = size / 2;
  int decomp_size = decoder.decode_index(destination_partial, buffer,
          compress_size_two, position);
  for (int i = 0; i < block_size; i++) {
    ASSERT_EQ(source[i], destination_partial[i]);
  }
 
}

#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

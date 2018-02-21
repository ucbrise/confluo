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
  size_t size = 10000;
  size_t bytes_per_block = 1000;
  char source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  lz4_encode encoder(source, size, bytes_per_block);
  lz4_decode decoder(&encoder);

  char destination[size];
  decoder.decode_full(destination, size);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
  size_t size = 1000;
  size_t bytes_per_block = 100;
  char source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  lz4_encode encoder(source, size, bytes_per_block);
  lz4_decode decoder(&encoder);

  size_t dest_size = bytes_per_block + 30;
  int src_index = 210;

  char destination[dest_size];
  decoder.decode_partial(destination, bytes_per_block, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

}

TEST_F(LZ4EncodeTest, EncodeDecodeIndexTest) {

  size_t size = 10000;
  size_t bytes_per_block = 1000;
  char source[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i;
  }

  lz4_encode encoder(source, size, bytes_per_block);
  lz4_decode decoder(&encoder);

  int src_index = 230;
  size_t dest_size = size - src_index;

  char destination[dest_size];
  decoder.decode_index(destination, bytes_per_block, src_index);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

 
}

#endif /* CONFLUO_TEST_DELTA_ENCODED_ARRAY_TEST_H_ */

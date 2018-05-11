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
  size_t size = 11048;
  uint8_t* source = new uint8_t[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  uint8_t* destination = new uint8_t[size];

  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(source[i], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
  size_t size = 11048;
  uint8_t* source = new uint8_t[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  size_t dest_size = BYTES_PER_BLOCK * 2 + 210;
  int src_index = 7210;

  uint8_t* destination = new uint8_t[dest_size];

  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialLessThanBlockSizeTest) {
  size_t size = 11048;
  uint8_t* source = new uint8_t[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  size_t dest_size = 210;
  int src_index = 7210;
  size_t encode_buffer_size = lz4_encoder<>::get_buffer_size(size);

  uint8_t *destination = new uint8_t[dest_size];
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }
  
  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodePartialLastIndexTest) {
  size_t size = 11048;
  uint8_t* source = new uint8_t[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  size_t dest_size = 790;
  int src_index = 9210;

  uint8_t *destination = new uint8_t[dest_size];
  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);

  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index, dest_size);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

TEST_F(LZ4EncodeTest, EncodeDecodeIndexTest) {
  size_t size = 11048;
  uint8_t* source = new uint8_t[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  int src_index = 7210;

  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  uint8_t decoded_value = lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(),
                                                               src_index);

  ASSERT_EQ(source[src_index], decoded_value);

  delete[] source;

}

TEST_F(LZ4EncodeTest, EncodeDecodeIdxPtrTest) {

  size_t size = 11048;
  uint8_t* source = new uint8_t[size];

  for (size_t i = 0; i < size; i++) {
    source[i] = i % 256;
  }

  int src_index = 230;
  size_t dest_size = size - src_index;

  uint8_t* destination = new uint8_t[dest_size];

  auto encoded_buffer = lz4_encoder<BYTES_PER_BLOCK>::encode(source, size);
  lz4_decoder<BYTES_PER_BLOCK>::decode(encoded_buffer.get(), destination, src_index);

  for (size_t i = 0; i < dest_size; i++) {
    ASSERT_EQ(source[i + src_index], destination[i]);
  }

  delete[] source;
  delete[] destination;
}

#endif /* CONFLUO_TEST_LZ4_ENCODE_TEST_H_ */

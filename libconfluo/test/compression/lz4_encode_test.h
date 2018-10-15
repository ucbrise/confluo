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
    // BYTES_PER_BLOCK is by default 65536 so this is for non-large block sizes
    static const size_t LARGE_SOURCE_BUFFER_SIZE = 11048;
    static const size_t SMALL_SOURCE_BUFFER_SIZE = 2048;
    static const size_t DIVISIBLE_SOURCE_BUFFER_SIZE = 12000;
};

size_t const LZ4EncodeTest::BYTES_PER_BLOCK;

/**
 * Test cases where source buffer size > block size
 */

TEST_F(LZ4EncodeTest, EncodeDecodeFullTest) {
  // initialized array size
  size_t size = LARGE_SOURCE_BUFFER_SIZE;
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

TEST_F(LZ4EncodeTest, EncodeDecodePartialTest) {
    // initialized array size
    size_t size = LARGE_SOURCE_BUFFER_SIZE;
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
    size_t size = LARGE_SOURCE_BUFFER_SIZE;
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
    size_t size = LARGE_SOURCE_BUFFER_SIZE;
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
    size_t size = LARGE_SOURCE_BUFFER_SIZE;
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
    size_t size = LARGE_SOURCE_BUFFER_SIZE;
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

/**
 * Test cases where source buffer size < block size
 */
TEST_F(LZ4EncodeTest, EncodeDecodeFullBigBlockTest) {
  // initialized array size
  size_t size = SMALL_SOURCE_BUFFER_SIZE;
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


TEST_F(LZ4EncodeTest, EncodeDecodePartialBigBlockTest) {
    // initialized array size
    size_t size = SMALL_SOURCE_BUFFER_SIZE;
    uint8_t *source = new uint8_t[size];

    // populates starting array with indexes modded by 256
    for (size_t i = 0; i < size; i++) {
        source[i] = i % 256;
    }

    size_t dest_size = 500;
    int src_index = 1500;

    // initializes destination array of smaller size
    uint8_t *destination = new uint8_t[dest_size];
    // encodes the starting array into buffer
    auto encoded_buffer = lz4_encoder<>::encode(source, size);
    // partially decodes (dest_size) the buffer into destination array from the index
    lz4_decoder<>::decode(encoded_buffer.get(), destination, src_index, dest_size);

    // checks to see if starting array and destination array have same elements
    for (size_t i = 0; i < dest_size; i++) {
        ASSERT_EQ(source[i + src_index], destination[i]);
    }

    delete[] source;
    delete[] destination;
}

// don't need a EncodeDecodePartialLessThanBlockSizeTest for BigBlock because the dest_size will be smaller regardless

TEST_F(LZ4EncodeTest, EncodeDecodeIndexBigBlockTest) {
    // initialized array size
    size_t size = SMALL_SOURCE_BUFFER_SIZE;
    uint8_t *source = new uint8_t[size];

    // populates starting array with indexes modded by 256
    for (size_t i = 0; i < size; i++) {
        source[i] = i % 256;
    }

    int src_index = 1000;

    // encodes the starting array into buffer
    auto encoded_buffer = lz4_encoder<>::encode(source, size);
    // decodes one byte at the index
    uint8_t decoded_value = lz4_decoder<>::decode(encoded_buffer.get(), src_index);

    //checks to see if the decoded one byte is the same as the starting
    ASSERT_EQ(source[src_index], decoded_value);

    delete[] source;

}

TEST_F(LZ4EncodeTest, EncodeDecodeIdxPtrBigBlockTest) {
    // initialized array size
    size_t size = SMALL_SOURCE_BUFFER_SIZE;
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
    auto encoded_buffer = lz4_encoder<>::encode(source, size);
    // decodes the buffer into destination array from the index
    lz4_decoder<>::decode(encoded_buffer.get(), destination, src_index);

    // checks to see if starting array and destination array have same elements
    for (size_t i = 0; i < dest_size; i++) {
        ASSERT_EQ(source[i + src_index], destination[i]);
    }

    delete[] source;
    delete[] destination;
}

/**
 * Test cases where source buffer size % block size
 */

TEST_F(LZ4EncodeTest, EncodeDecodeFullDivisibleBlockTest) {
    // initialized array size
    size_t size = DIVISIBLE_SOURCE_BUFFER_SIZE;
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

TEST_F(LZ4EncodeTest, EncodeDecodePartialDivisibleBlockTest) {
    // initialized array size
    size_t size = DIVISIBLE_SOURCE_BUFFER_SIZE;
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


TEST_F(LZ4EncodeTest, EncodeDecodePartialLessThanBlockSizeDivisibleBlockTest) {
    // initialized array size
    size_t size = DIVISIBLE_SOURCE_BUFFER_SIZE;
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

TEST_F(LZ4EncodeTest, EncodeDecodePartialLastIndexDivisibleBlockTest) {
    // initialized array size
    size_t size = DIVISIBLE_SOURCE_BUFFER_SIZE;
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

TEST_F(LZ4EncodeTest, EncodeDecodeIndexDivisibleBlockTest) {
    // initialized array size
    size_t size = DIVISIBLE_SOURCE_BUFFER_SIZE;
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

TEST_F(LZ4EncodeTest, EncodeDecodeIdxPtrDivisibleBlockTest) {
    // initialized array size
    size_t size = DIVISIBLE_SOURCE_BUFFER_SIZE;
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

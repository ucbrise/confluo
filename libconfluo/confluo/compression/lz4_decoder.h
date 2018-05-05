#ifndef CONFLUO_COMPRESSION_LZ4_DECODER_H_
#define CONFLUO_COMPRESSION_LZ4_DECODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <math.h>

#include "lz4.h"

namespace confluo {
namespace compression {

/**
 * A stateless decoder that decodes partial or full information for an
 * LZ4 encoded byte stream
 */
template<size_t BYTES_PER_BLOCK = 65536>
class lz4_decoder {
 public:
  /**
   * Decodes a length number of bytes from the src_index position in the
   * input buffer
   *
   * @param input_buffer The LZ4 encoded buffer
   * @param encode_size The size of the LZ4 encoded buffer
   * @param source_size The size of the unencoded buffer
   * @param buffer The destination buffer to be filled with decoded data
   * @param src_index The index into the unencoded buffer to begin
   * decoding
   * @param length The number of bytes to decode
   */
  static void decode(uint8_t* input_buffer, size_t encode_size, uint8_t* dest_buffer, size_t src_index, size_t length) {
    size_t decoded_buf_size = decoded_size(input_buffer);
    encode_size -= sizeof(size_t);
    input_buffer += sizeof(size_t);

    int compress_index = src_index / BYTES_PER_BLOCK;
    int position_within_block = src_index % BYTES_PER_BLOCK;

    size_t offset = *reinterpret_cast<size_t*>(input_buffer + compress_index * sizeof(size_t));
    uint8_t* temp_buffer = new uint8_t[BYTES_PER_BLOCK];

    size_t compress_size = 0;
    int max_index = decoded_buf_size / BYTES_PER_BLOCK;

    if (compress_index + 1 < max_index) {
      compress_size = *reinterpret_cast<size_t*>(input_buffer +
                      (compress_index + 1) * sizeof(size_t)) - offset;
    } else {
      compress_size = encode_size - offset;
    }

    LZ4_decompress_safe((char*) input_buffer + offset, (char*) temp_buffer, compress_size, BYTES_PER_BLOCK);
    size_t len = 0;
    if (position_within_block + length < BYTES_PER_BLOCK) {
      len = length;
    } else {
      len = BYTES_PER_BLOCK - position_within_block;
    }
    std::memcpy(dest_buffer, temp_buffer + position_within_block, len);
    length -= len;
    compress_index++;
    delete[] temp_buffer;

    while (length >= BYTES_PER_BLOCK) {
      offset = *reinterpret_cast<size_t*>(input_buffer + compress_index * sizeof(size_t));
      if (compress_index + 1 < max_index) {
        compress_size = *reinterpret_cast<size_t*>(input_buffer +
                                                   (compress_index + 1) * sizeof(size_t)) - offset;
      } else {
        compress_size = encode_size - offset;
      }

      LZ4_decompress_safe((char*) input_buffer + offset,
                          (char*) dest_buffer + len, compress_size, BYTES_PER_BLOCK);
      len += BYTES_PER_BLOCK;
      length -= BYTES_PER_BLOCK;
      compress_index++;
    }

    if (length > 0) {
      uint8_t* tail = new uint8_t[BYTES_PER_BLOCK];
   
      offset = *reinterpret_cast<size_t*>(input_buffer +
            compress_index * sizeof(size_t));

      if (compress_index + 1 < max_index) {
        compress_size = *reinterpret_cast<size_t*>(input_buffer +
                (compress_index + 1) * sizeof(size_t)) - offset;
      } else {
        compress_size = encode_size - offset;
      }

      LZ4_decompress_safe((char*) input_buffer + offset,
            (char*) tail, compress_size, BYTES_PER_BLOCK);
      std::memcpy(dest_buffer + len, tail, length);
      delete[] tail;
    }

  }

//  /**
//   * Decodes all of the bytes present in the LZ4 encoded buffer
//   *
//   * @param input_buffer The encoded input buffer
//   * @param encode_size The size of the encoded buffer
//   * @param bytes_per_block The number of unencoded bytes corresponding to
//   * one encoded block
//   * @param buffer The buffer to contain the decoded data
//   */
//  static void decode(uint8_t* input_buffer, size_t encode_size,
//                     uint8_t* buffer, size_t bytes_per_block = 65536) {
//    int num_offsets = ceil(decoded_size(input_buffer) / bytes_per_block);
//    size_t* offsets = new size_t[num_offsets];
//
//    for (int i = 0; i < num_offsets; i++) {
//      size_t offset = *reinterpret_cast<size_t*>(input_buffer + i * sizeof(size_t));
//      uint8_t* block = input_buffer + offset;
//      size_t compressed_size = 0;
//      if (i + 1 < num_offsets) {
//        compressed_size = *reinterpret_cast<size_t*>(input_buffer + (i + 1) * sizeof(size_t)) - offset;
//      } else {
//        compressed_size = encode_size - offset;
//      }
//      LZ4_decompress_safe((char*) block, (char*) (buffer + i * bytes_per_block), compressed_size, bytes_per_block);
//    }
//
//    delete[] offsets;
//
//  }

  /**
   * Decodes one byte corresponding to an index specified in the unencoded
   * buffer
   *
   * @param input_buffer The LZ4 encoded buffer to decode
   * @param src_index The index into the unencoded buffer to decode from
   * @param encode_size The number of bytes the LZ4 encoded buffer contains
   * @param bytes_per_block The number of bytes in the unencoded buffer corresponding to one encoded block
   *
   * @return The decoded byte
   */
  static uint8_t decode(uint8_t* input_buffer, size_t encode_size, size_t src_index) {

    size_t decoded_buf_size = decoded_size(input_buffer);
    encode_size -= sizeof(size_t);
    input_buffer += sizeof(size_t);

    int compress_index = src_index / BYTES_PER_BLOCK;
    int position_within_block = src_index % BYTES_PER_BLOCK;

    size_t offset = *reinterpret_cast<size_t*>(input_buffer + compress_index * sizeof(size_t));
    uint8_t* temp_buffer = new uint8_t[BYTES_PER_BLOCK];

    size_t compress_size = 0;
    int max_index = decoded_buf_size / BYTES_PER_BLOCK;

    if (compress_index + 1 < max_index) {
      compress_size = *reinterpret_cast<size_t*>(input_buffer +
                      (compress_index + 1) * sizeof(size_t)) - offset;
    } else {
      compress_size = encode_size - offset;
    }

    LZ4_decompress_safe((char*) input_buffer + offset, (char*) temp_buffer, compress_size, BYTES_PER_BLOCK);
    uint8_t val;
    std::memcpy(&val, &temp_buffer[position_within_block], sizeof(uint8_t));
    delete[] temp_buffer;
    return val;
  }

  /**
   * Decodes the whole LZ4 encoded pointer starting from a particular index
   *
   * @param input_buffer The LZ4 encoded buffer
   * @param encode_size The size of the encoded buffer in bytes
   * @param dest_buffer The buffer to contain the decoded data
   * @param src_index The index into the unencoded buffer to start decoding
   * the pointer from
   */
  static void decode(uint8_t* input_buffer, size_t encode_size, uint8_t* dest_buffer, size_t src_index = 0) {
    size_t length = decoded_size(input_buffer) - src_index;
    decode(input_buffer, encode_size, dest_buffer, src_index, length);
  }

  static size_t decoded_size(uint8_t* input_buffer) {
    return *reinterpret_cast<size_t*>(input_buffer);
  }

};

}
}

#endif /* CONFLUO_COMPRESSION_LZ4_DECODER_H_ */

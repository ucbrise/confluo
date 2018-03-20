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
class lz4_decoder {
 public:
  /**
   * Decodes a length number of bytes from the src_index position in the
   * input buffer
   *
   * @param input_buffer The LZ4 encoded buffer
   * @param encode_size The size of the LZ4 encoded buffer
   * @param source_size The size of the unencoded buffer
   * @param bytes_per_block The number of unencoded bytes corresponding to
   * one encoded block
   * @param buffer The destination buffer to be filled with decoded data
   * @param src_index The index into the unencoded buffer to begin
   * decoding
   * @param length The number of bytes to decode
   */
  static void decode(uint8_t* input_buffer, size_t encode_size, size_t source_size, uint8_t* buffer,
                     int src_index, size_t length, size_t bytes_per_block = 65536) {
    int compress_index = src_index / bytes_per_block;
    int position_within_block = src_index % bytes_per_block;

    size_t offset = *reinterpret_cast<size_t*>(input_buffer + compress_index * sizeof(size_t));
    uint8_t* temp_buffer = new uint8_t[bytes_per_block];

    size_t compress_size = 0;
    int max_index = source_size / bytes_per_block;

    if (compress_index + 1 < max_index) {
      compress_size = *reinterpret_cast<size_t*>(input_buffer +
                      (compress_index + 1) * sizeof(size_t)) - offset;
    } else {
      compress_size = encode_size - offset;
    }

    LZ4_decompress_safe((char*) input_buffer + offset, (char*) temp_buffer, compress_size, bytes_per_block);
    size_t len = 0;
    if (position_within_block + length < bytes_per_block) {
      len = length;
    } else {
      len = bytes_per_block - position_within_block;
    }
    std::memcpy(buffer, temp_buffer + position_within_block, len);
    length -= len;
    compress_index++;
    delete[] temp_buffer;

    while (length >= bytes_per_block) {
      offset = *reinterpret_cast<size_t*>(input_buffer + compress_index * sizeof(size_t));
      if (compress_index + 1 < max_index) {
        compress_size = *reinterpret_cast<size_t*>(input_buffer +
                                                   (compress_index + 1) * sizeof(size_t)) - offset;
      } else {
        compress_size = encode_size - offset;
      }

      LZ4_decompress_safe((char*) input_buffer + offset,
                          (char*) buffer + len, compress_size, bytes_per_block);
      len += bytes_per_block;
      length -= bytes_per_block;
      compress_index++;
    }

    if (length > 0) {
      uint8_t* tail = new uint8_t[bytes_per_block];
   
      offset = *reinterpret_cast<size_t*>(input_buffer +
            compress_index * sizeof(size_t));

      if (compress_index + 1 < max_index) {
        compress_size = *reinterpret_cast<size_t*>(input_buffer +
                (compress_index + 1) * sizeof(size_t)) - offset;
      } else {
        compress_size = encode_size - offset;
      }

      LZ4_decompress_safe((char*) input_buffer + offset,
            (char*) tail, compress_size, bytes_per_block);
      std::memcpy(buffer + len, tail, length);
      delete[] tail;
    }

  }

  /**
   * Decodes all of the bytes present in the LZ4 encoded buffer
   *
   * @param input_buffer The encoded input buffer
   * @param encode_size The size of the encoded buffer
   * @param source_length The length in bytes of the unencoded buffer
   * @param bytes_per_block The number of unencoded bytes corresponding to 
   * one encoded block
   * @param buffer The buffer to contain the decoded data
   */
  static void decode(uint8_t* input_buffer, size_t encode_size, size_t source_length,
                     uint8_t* buffer, size_t bytes_per_block = 65536) {
    int num_offsets = ceil(source_length / bytes_per_block);
    size_t* offsets = new size_t[num_offsets];

    for (int i = 0; i < num_offsets; i++) {
      size_t offset = *reinterpret_cast<size_t *>(input_buffer + i * sizeof(size_t));
      uint8_t* block = input_buffer + offset;
      size_t compressed_size = 0;
      if (i + 1 < num_offsets) {
        compressed_size = *reinterpret_cast<size_t *>(input_buffer + (i + 1) * sizeof(size_t)) - offset;
      } else {
        compressed_size = encode_size - offset;
      }
      LZ4_decompress_safe((char*) block, (char*) (buffer + i * bytes_per_block), compressed_size, bytes_per_block);
    }

    delete[] offsets;
      
  }

  /**
   * Decodes one byte corresponding to an index specified in the unencoded
   * buffer
   *
   * @param input_buffer The LZ4 encoded buffer to decode
   * @param encode_size The number of bytes the LZ4 encoded buffer contains
   * @param bytes_per_block The number of bytes in the unencoded buffer
   * corresponding to one encoded block
   * @param source_size The size of the unencoded buffer in bytes
   * @param src_index The index into the unencoded buffer to decode from
   *
   * @return The decoded byte
   */
  static uint8_t decode(uint8_t* input_buffer, size_t encode_size, size_t source_size,
                        int src_index, size_t bytes_per_block = 65536) {
    int compress_index = src_index / bytes_per_block;
    int position_within_block = src_index % bytes_per_block;

    size_t offset = *reinterpret_cast<size_t*>(input_buffer + compress_index * sizeof(size_t));
    uint8_t* temp_buffer = new uint8_t[bytes_per_block];

    size_t compress_size = 0;
    int max_index = source_size / bytes_per_block;

    if (compress_index + 1 < max_index) {
      compress_size = *reinterpret_cast<size_t*>(input_buffer +
                      (compress_index + 1) * sizeof(size_t)) - offset;
    } else {
      compress_size = encode_size - offset;
    }

    LZ4_decompress_safe((char*) input_buffer + offset, (char*) temp_buffer, compress_size, bytes_per_block);
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
   * @param source_size The size of the unencoded buffer in bytes
   * @param bytes_per_block The number of bytes in the unencoded buffer
   * that correspond to one encoded block
   * @param buffer The buffer to contain the decoded data
   * @param src_index The index into the unencoded buffer to start decoding
   * the pointer from
   */
  static void decode(uint8_t* input_buffer, size_t encode_size, size_t source_size,
                     size_t bytes_per_block, uint8_t* buffer, int src_index) {
    size_t length = source_size - src_index;
    decode(input_buffer, encode_size, source_size, buffer, src_index, length, bytes_per_block);
  }

};

}
}

#endif /* CONFLUO_COMPRESSION_LZ4_DECODER_H_ */

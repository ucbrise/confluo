#ifndef CONFLUO_CONTAINER_BITMAP_LZ4_DECODER_H_
#define CONFLUO_CONTAINER_BITMAP_LZ4_DECODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <math.h>

#include "lz4.h"

namespace confluo {

class lz4_decoder {
 public:
  /**
   * Decodes starting from the given index for length number of bytes
   */
  static void decode_partial(uint8_t* input_buffer, size_t encode_size, size_t source_size, size_t bytes_per_block, uint8_t* buffer, int src_index, size_t length) {
    int compress_index = src_index / bytes_per_block;
    int position_within_block = src_index % bytes_per_block;

    size_t offset = *reinterpret_cast<size_t *>(input_buffer + 
            compress_index * sizeof(size_t));
    uint8_t temp_buffer[bytes_per_block];

    size_t compress_size = 0;
    if (compress_index + 1 < source_size / bytes_per_block) {
      compress_size = *reinterpret_cast<size_t *>(input_buffer + 
            (compress_index + 1) * sizeof(size_t)) - offset;
    } else {
      compress_size = encode_size - offset;
    }

    LZ4_decompress_safe((char *) input_buffer + offset, 
            (char *) temp_buffer, compress_size, bytes_per_block);
    size_t len = 0;
    if (position_within_block + length < bytes_per_block) {
      len = length;
    } else {
      len = bytes_per_block - position_within_block;
    }
    std::memcpy(buffer, temp_buffer + position_within_block, len);
    length -= len;
    compress_index++;

    while (length >= bytes_per_block) {
      offset = *reinterpret_cast<size_t *>(input_buffer + 
            compress_index * sizeof(size_t));
      if (compress_index + 1 < source_size / bytes_per_block) {
        compress_size = *reinterpret_cast<size_t *>(input_buffer + 
                (compress_index + 1) * sizeof(size_t)) - offset;
      } else {
        compress_size = encode_size - offset;
      }

      LZ4_decompress_safe((char *) input_buffer + offset,
               (char *) buffer + len, compress_size, bytes_per_block);
      len += bytes_per_block;
      length -= bytes_per_block;
      compress_index++;
    }

    if (length > 0) {
      uint8_t tail[bytes_per_block];
   
      offset = *reinterpret_cast<size_t *>(input_buffer + 
            compress_index * sizeof(size_t));

      if (compress_index + 1 < source_size / bytes_per_block) {
        compress_size = *reinterpret_cast<size_t *>(input_buffer + 
                (compress_index + 1) * sizeof(size_t)) - offset;
      } else {
        compress_size = encode_size - offset;
      }

      LZ4_decompress_safe((char*) input_buffer + offset,
            (char*) tail, compress_size, bytes_per_block);
      std::memcpy(buffer + len, tail, length);
    }


  }

  /**
   * Decodes everything
   */
  static void decode_full(uint8_t* input_buffer, size_t encode_size, size_t source_length, size_t bytes_per_block, uint8_t* buffer) {

    int num_offsets = ceil(source_length / bytes_per_block);
    size_t offsets[num_offsets];

    for (size_t i = 0; i < num_offsets; i++) {
      size_t offset = *reinterpret_cast<size_t *>(input_buffer + i * sizeof(size_t));

      uint8_t* block = input_buffer + offset;
      size_t compressed_size = 0;

      if (i + 1 < num_offsets) {
        compressed_size = *reinterpret_cast<size_t *>(input_buffer + (i + 1) * sizeof(size_t)) - offset;

      } else {
        compressed_size = encode_size - offset;
      }
      LZ4_decompress_safe((char *) block, (char *) (buffer + i * bytes_per_block), compressed_size, bytes_per_block);
    }
      
  }

  /**
   * Decodes only one byte at the given index
   */
  static uint8_t decode_index(uint8_t* input_buffer, size_t encode_size, size_t bytes_per_block, size_t source_size, int src_index) {
    
    int compress_index = src_index / bytes_per_block;
    int position_within_block = src_index % bytes_per_block;

    size_t offset = *reinterpret_cast<size_t *>(input_buffer + 
            compress_index * sizeof(size_t));
    uint8_t temp_buffer[bytes_per_block];

    size_t compress_size = 0;
    if (compress_index + 1 < source_size / bytes_per_block) {
      compress_size = *reinterpret_cast<size_t *>(input_buffer + 
            (compress_index + 1) * sizeof(size_t)) - offset;
    } else {
      compress_size = encode_size - offset;
    }

    LZ4_decompress_safe((char *) input_buffer + offset, 
            (char *) temp_buffer, compress_size, bytes_per_block);
    return temp_buffer[position_within_block];
  }

  /**
   * Decodes the whole pointer starting from the given index
   */
  static void decode_index_ptr(uint8_t* input_buffer, size_t encode_size, size_t source_size, size_t bytes_per_block, uint8_t* buffer, int src_index) {
    size_t length = source_size - src_index;
    decode_partial(input_buffer, encode_size, source_size, bytes_per_block,
            buffer, src_index, length);
  }

};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_DECODER_H_ */

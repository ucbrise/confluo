#ifndef CONFLUO_CONTAINER_BITMAP_LZ4_DECODER_H_
#define CONFLUO_CONTAINER_BITMAP_LZ4_DECODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "lz4.h"

namespace confluo {

class lz4_decoder {
 public:
  lz4_decoder() {
  }
  
  void decode_partial(std::vector<uint8_t*>* compressed_blocks, std::vector<size_t>* compressed_sizes, size_t bytes_per_block, uint8_t* buffer, size_t buffer_size, int src_index, size_t length) {
    int compress_index = src_index / bytes_per_block;
    int position_within_block = src_index % bytes_per_block;

    uint8_t temp_buffer[bytes_per_block];
    LZ4_decompress_safe((char*) compressed_blocks->at(compress_index), 
              (char*) temp_buffer, compressed_sizes->at(compress_index),
              bytes_per_block);

    size_t len = bytes_per_block - position_within_block;
    std::memcpy(buffer, temp_buffer + position_within_block, len);
    length -= len;
    compress_index++;

    while (length >= bytes_per_block) {
      LZ4_decompress_safe((char*) compressed_blocks->at(compress_index), 
              (char*) (buffer + len), compressed_sizes->at(compress_index),
              bytes_per_block);

      len += bytes_per_block;
      length -= bytes_per_block;
      compress_index++;
    }

    if (length > 0) {
      uint8_t tail[bytes_per_block];
      LZ4_decompress_safe((char*) compressed_blocks->at(compress_index),
            (char*) tail, compressed_sizes->at(compress_index),
            bytes_per_block);
      std::memcpy(buffer + len, tail, length);
    }
  }

  void decode_full(std::vector<uint8_t*>* compressed_blocks, std::vector<size_t>* compressed_sizes, size_t bytes_per_block, uint8_t* buffer, size_t buffer_size) {
    for (size_t i = 0; i < compressed_blocks->size(); i++) {
      LZ4_decompress_safe((char*) compressed_blocks->at(i), 
              (char*) (buffer + i * bytes_per_block), 
              compressed_sizes->at(i),
              bytes_per_block);
    }
      
  }

  void decode_index(std::vector<uint8_t*>* compressed_blocks, std::vector<size_t>* compressed_sizes, size_t bytes_per_block, uint8_t* buffer, size_t buffer_size, int src_index) {
    int compress_index = src_index / bytes_per_block;
    int position_within_block = src_index % bytes_per_block;

    uint8_t temp_buffer[bytes_per_block];
    LZ4_decompress_safe((char*)compressed_blocks->at(compress_index), 
              (char*) temp_buffer, compressed_sizes->at(compress_index),
              bytes_per_block);

    size_t len = bytes_per_block - position_within_block;
    std::memcpy(buffer, temp_buffer + position_within_block, len);
    compress_index++;

    while (compress_index < compressed_blocks->size()) {
      LZ4_decompress_safe((char*)compressed_blocks->at(compress_index), 
              (char*) (buffer + len), compressed_sizes->at(compress_index),
              bytes_per_block);

      len += bytes_per_block;
      compress_index++;
    }

  }

};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_DECODER_H_ */

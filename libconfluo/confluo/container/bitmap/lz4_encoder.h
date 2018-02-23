#ifndef CONFLUO_CONTAINER_BITMAP_LZ4_ENCODER_H_
#define CONFLUO_CONTAINER_BITMAP_LZ4_ENCODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "lz4.h"

namespace confluo {

/**
 * Container for compressed blocks. Takes as input a buffer of bytes
 * and number of bytes per block parameter and splits up the input data
 * into separate blocks that are each compressed using LZ4. These 
 * compressed blocks are fed into the decoder for partial or full decoding.
 */
class lz4_encoder {
 public:
  lz4_encoder() {
  }

  ~lz4_encoder() {
  }

  size_t encode(uint8_t* source_buffer, size_t source_length, size_t bytes_per_block, std::vector<uint8_t*>* compressed_blocks, std::vector<size_t>* compressed_sizes) {

    for (size_t i = 0; i < source_length; i += bytes_per_block) {
      uint8_t* compress = new uint8_t[LZ4_compressBound(bytes_per_block)];
      size_t size = encode((char*) compress, (char*) (source_buffer + i), 
              bytes_per_block);
      compressed_blocks->push_back(compress);
      compressed_sizes->push_back(size);
    }
    return get_buffer_size(source_length);
  }

  size_t encode(char* buffer, char* source, size_t source_size) {
    return LZ4_compress_default(source, buffer, source_size, get_buffer_size(source_size));
  }

  size_t get_buffer_size(size_t source_size) {
    return LZ4_compressBound(source_size);
  }

};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_ENCODER_H_ */

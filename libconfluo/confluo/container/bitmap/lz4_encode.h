#ifndef CONFLUO_CONTAINER_BITMAP_LZ4_ENCODE_H_
#define CONFLUO_CONTAINER_BITMAP_LZ4_ENCODE_H_

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
class lz4_encode {
 public:
  lz4_encode(char* source_buffer, size_t source_length, 
          size_t bytes_per_block) {
    bytes_per_block_ = bytes_per_block;

    for (size_t i = 0; i < source_length; i += bytes_per_block) {
      char* compress = new char[LZ4_compressBound(bytes_per_block)];
      size_t size = encode(compress, source_buffer + i, bytes_per_block);
      compressed_blocks_.push_back(compress);
      compressed_sizes_.push_back(size);
    }
  }

  ~lz4_encode() {
    for (size_t i = 0; i < compressed_blocks_.size(); i++) {
      delete compressed_blocks_[i];
    }
  }

  size_t encode(char* buffer, char* source, size_t source_size) {
    return LZ4_compress_default(source, buffer, source_size, 
            get_buffer_size(source_size));
  }

  size_t get_buffer_size(size_t source_size) {
    return LZ4_compressBound(source_size);
  }

  std::vector<char *>* get_compressed_blocks() {
    return &compressed_blocks_;
  }

  std::vector<size_t>* get_compressed_sizes() {
    return &compressed_sizes_;
  }

  size_t bytes_per_block() {
    return bytes_per_block_;
  }

 private:
  std::vector<char *> compressed_blocks_;
  std::vector<size_t> compressed_sizes_;
  size_t bytes_per_block_;

};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_COMPRESSION_H_ */

#ifndef CONFLUO_CONTAINER_BITMAP_LZ4_DECODE_H_
#define CONFLUO_CONTAINER_BITMAP_LZ4_DECODE_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "lz4.h"

namespace confluo {

class lz4_decode {
 public:
  lz4_decode(lz4_encode* encoder) {
    encoder_ = encoder;
  }
  
  void decode_partial(char* buffer, size_t buffer_size, int src_index, size_t length) {
    int compress_index = src_index / encoder_->bytes_per_block();
    int position_within_block = src_index % encoder_->bytes_per_block();

    std::vector<char *>* compressed_blocks = encoder_->get_compressed_blocks();
    std::vector<size_t>* compressed_sizes = encoder_->get_compressed_sizes();

    char temp_buffer[encoder_->bytes_per_block()];
    LZ4_decompress_safe(compressed_blocks->at(compress_index), 
              temp_buffer, compressed_sizes->at(compress_index),
              encoder_->bytes_per_block());

    size_t len = encoder_->bytes_per_block() - position_within_block;
    std::memcpy(buffer, temp_buffer + position_within_block, len);
    length -= len;
    compress_index++;

    while (length >= encoder_->bytes_per_block()) {
      LZ4_decompress_safe(compressed_blocks->at(compress_index), 
              buffer + len, compressed_sizes->at(compress_index),
              encoder_->bytes_per_block());

      len += encoder_->bytes_per_block();
      length -= encoder_->bytes_per_block();
      compress_index++;
    }

    if (length > 0) {
      char tail[encoder_->bytes_per_block()];
      LZ4_decompress_safe(compressed_blocks->at(compress_index),
            tail, compressed_sizes->at(compress_index),
            encoder_->bytes_per_block());
      std::memcpy(buffer + len, tail, length);
    }
  }

  void decode_full(char* buffer, size_t buffer_size) {
    size_t bytes_per_block = encoder_->bytes_per_block();
    std::vector<char *>* compressed_blocks = 
        encoder_->get_compressed_blocks();
    std::vector<size_t>* compressed_sizes = encoder_->get_compressed_sizes();
    for (size_t i = 0; i < compressed_blocks->size(); i++) {
      LZ4_decompress_safe(compressed_blocks->at(i), 
              buffer + i * bytes_per_block, compressed_sizes->at(i),
              bytes_per_block);
    }
      
  }

  void decode_index(char* buffer, size_t buffer_size, int src_index) {
    int compress_index = src_index / encoder_->bytes_per_block();
    int position_within_block = src_index % encoder_->bytes_per_block();

    std::vector<char *>* compressed_blocks = encoder_->get_compressed_blocks();
    std::vector<size_t>* compressed_sizes = encoder_->get_compressed_sizes();

    char temp_buffer[encoder_->bytes_per_block()];
    LZ4_decompress_safe(compressed_blocks->at(compress_index), 
              temp_buffer, compressed_sizes->at(compress_index),
              encoder_->bytes_per_block());

    size_t len = encoder_->bytes_per_block() - position_within_block;
    std::memcpy(buffer, temp_buffer + position_within_block, len);
    compress_index++;

    while (compress_index < compressed_blocks->size()) {
      LZ4_decompress_safe(compressed_blocks->at(compress_index), 
              buffer + len, compressed_sizes->at(compress_index),
              encoder_->bytes_per_block());

      len += encoder_->bytes_per_block();
      compress_index++;
    }

  }

 private:
  lz4_encode* encoder_;
};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_COMPRESSION_H_ */

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
  lz4_decode() {
  }

  
  size_t decode_partial(char* buffer, char* source, int compressed_size,
          int target_size, int src_index) {
    return LZ4_decompress_safe_partial(source + src_index, buffer, compressed_size, target_size, compressed_size);
  }

  size_t decode_full(char* buffer, char* source, int compress_size, int size) {
    return LZ4_decompress_safe(source, buffer, compress_size, size);
  }

  size_t decode_index(char* buffer, char* source, int src_index, int compress_size) {
    return decode_partial(buffer, source, compress_size - src_index, compress_size, src_index);
  }
};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_COMPRESSION_H_ */

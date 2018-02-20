#ifndef CONFLUO_CONTAINER_BITMAP_LZ4_ENCODE_H_
#define CONFLUO_CONTAINER_BITMAP_LZ4_ENCODE_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "lz4.h"

namespace confluo {

class lz4_encode {
 public:
  lz4_encode() {
    
  }

  size_t encode(char* buffer, char* source, size_t source_size) {
    return LZ4_compress_default(source, buffer, source_size, 
            get_buffer_size(source_size));
  }

  size_t get_buffer_size(size_t source_size) {
    return LZ4_compressBound(source_size);
  }

};

}

#endif /* CONFLUO_CONTAINER_BITMAP_LZ4_COMPRESSION_H_ */

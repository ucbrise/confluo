#ifndef CONFLUO_COMPRESSION_DELTA_ENCODER_H_
#define CONFLUO_COMPRESSION_DELTA_ENCODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
#include "container/bitmap/delta_encoded_array.h"
#include "container/unique_byte_array.h"

namespace confluo {
namespace compression {

/**
 * A stateless encoder. Takes as input a buffer of bytes and applies delta
 * encoding on that data. Supports full encoding and packaged for full
 * or partial decoding
 */
class delta_encoder {
 public:
  /**
   * Encodes the input buffer data using delta encoding
   *
   * @param source_buffer The buffer data to encode
   * @param source_length The length of the input buffer array
   */
  template<typename T>
  static unique_byte_array encode(T* source_buffer, size_t source_length) {
    std::sort(source_buffer, source_buffer + source_length);
    elias_gamma_encoded_array<uint64_t> enc_array(source_buffer, source_length);
    size_t buffer_size = enc_array.storage_size() + sizeof(size_t);
    uint8_t* output_buffer = new uint8_t[buffer_size];
    std::memcpy(output_buffer, &source_length, sizeof(size_t));
    enc_array.to_byte_array(output_buffer + sizeof(size_t));
    return unique_byte_array(output_buffer, buffer_size);
  }

};

}
}

#endif /* CONFLUO_COMPRESSION_DELTA_ENCODER_H_ */

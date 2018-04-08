#ifndef CONFLUO_COMPRESSION_DELTA_ENCODER_H_
#define CONFLUO_COMPRESSION_DELTA_ENCODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <math.h>
#include "container/bitmap/delta_encoded_array.h"

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
  static uint8_t* encode(uint64_t* source_buffer, size_t source_length) {
    elias_gamma_encoded_array<uint64_t> enc_array(source_buffer, source_length);
    uint8_t* output_buffer = new uint8_t[enc_array.storage_size() + sizeof(size_t *)];

    size_t encode_size = enc_array.storage_size();
    std::memcpy(output_buffer, &encode_size, sizeof(size_t *));
    enc_array.to_byte_array(output_buffer + sizeof(size_t *));

    return output_buffer;
  }

  /**
   * Gets the size for the encoded buffer
   *
   * @param encoded_buffer The encoded buffer
   *
   * @return The size of the encoded buffer
   */
  static size_t get_buffer_size(uint8_t* encoded_buffer) {
    return *reinterpret_cast<size_t *>(encoded_buffer);
  }

};

}
}

#endif /* CONFLUO_COMPRESSION_DELTA_ENCODER_H_ */

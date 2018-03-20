#ifndef CONFLUO_COMPRESSION_DELTA_DECODER_H_
#define CONFLUO_COMPRESSION_DELTA_DECODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <math.h>

#include "container/bitmap/delta_encoded_array.h"

namespace confluo {
namespace compression {

/**
 * A stateless decoder. Takes as input a delta encoded input buffer and
 * performs partial or full decoding.
 */
class delta_decoder {
 public:
  /**
   * Decodes a single byte at a given index
   *
   * @param input_buffer The encoded buffer to decode
   * @param src_index The index to decode at
   * @param source_size The length of the non encoded buffer
   *
   * @return The decoded byte
   */
  static uint8_t decode(uint8_t* input_buffer, size_t src_index, size_t source_size) {
    elias_gamma_encoded_array<uint64_t> enc_array;
    enc_array.from_byte_array(input_buffer);
    return enc_array.get(src_index);
  }

  /**
   * Decodes a partial amount of the buffer starting from a specific index
   *
   * @param input_buffer The encoded buffer
   * @param dest_buffer The decoded buffer to contain the decoded bytes
   * @param src_index The index to start decoding from
   * @param length The number of bytes to decode
   * @param source_size The size of the unencoded buffer
   */
  static void decode(uint8_t* input_buffer, uint64_t* dest_buffer,
                     size_t src_index, size_t length, size_t source_size) {
    elias_gamma_encoded_array<uint64_t> enc_array;
    enc_array.from_byte_array(input_buffer);

    for (size_t i = 0; i < length; i++) {
      dest_buffer[i] = enc_array.get(src_index + i);
    }
  }

  /**
   * Decodes the full input buffer
   *
   * @param input_buffer The encoded buffer to decode
   * @param dest_buffer The decoded buffer to contain the decoded data
   * @param source_size The size of the unencoded array
   */
  static void decode(uint8_t* input_buffer, uint64_t* dest_buffer, size_t source_size) {
    elias_gamma_encoded_array<uint64_t> enc_array;
    enc_array.from_byte_array(input_buffer);

    for (size_t i = 0; i < source_size; i++) {
      dest_buffer[i] = enc_array.get(i);
    }
  }
      
  /**
   * Decodes the whole pointer starting from the specified index
   *
   * @param input_buffer The encoded buffer
   * @param src_index The index to start decoding from
   * @param source_size The size of the unencoded buffer
   * @param dest_buffer The buffer containing the decoded bytes
   */
  static void decode(uint8_t* input_buffer, size_t src_index, size_t source_size, uint64_t* dest_buffer) {
    elias_gamma_encoded_array<uint64_t> enc_array;
    enc_array.from_byte_array(input_buffer);

    for (size_t i = 0; i < source_size - src_index; i++) {
      dest_buffer[i] = enc_array.get(i + src_index);
    }
  }

};

}
}

#endif /* CONFLUO_COMPRESSION_DELTA_DECODER_H_ */

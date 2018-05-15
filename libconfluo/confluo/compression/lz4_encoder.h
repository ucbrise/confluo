#ifndef CONFLUO_COMPRESSION_LZ4_ENCODER_H_
#define CONFLUO_COMPRESSION_LZ4_ENCODER_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <math.h>

#include "lz4.h"
#include "container/unique_byte_array.h"

namespace confluo {
namespace compression {

/**
 * A stateless LZ4 encoder. Takes as input a buffer of bytes
 * and number of bytes per block parameter and splits up the input data
 * into separate blocks that are each compressed using LZ4. These 
 * compressed blocks are fed into the decoder for partial or full decoding.
 */
template<size_t BYTES_PER_BLOCK = 65536>
class lz4_encoder {
 public:
  /**
   * Encodes the input buffer using LZ4 compression
   *
   * @param source_buffer The unecoded buffer to compress
   * @param source_length The size of the unencoded buffer in bytes
   * @param output_buffer The output buffer containing the encoded data
   *
   * @return The size of the entire encoded buffer in bytes
   */
  static unique_byte_array encode(uint8_t* source_buffer, size_t source_length) {

    uint8_t* output_buffer = new uint8_t[get_buffer_size(source_length) + sizeof(size_t)];
    std::memcpy(output_buffer, &source_length, sizeof(size_t));
    output_buffer += sizeof(size_t);

    int num_blocks = ceil(source_length / BYTES_PER_BLOCK);
    size_t output_block_position = sizeof(size_t) * num_blocks + sizeof(size_t);
    size_t output_array_position = 0;
    for (size_t i = 0; i < source_length; i += BYTES_PER_BLOCK) {
      uint8_t* output_ptr = output_buffer + output_block_position;
      size_t size = encode((char*) output_ptr, (char*) (source_buffer + i), BYTES_PER_BLOCK);
      output_block_position += size;

      uint8_t* output_array_ptr = output_buffer + output_array_position;
      size_t bytes = sizeof(size_t);
      size_t offset = output_block_position - size;
      std::memcpy(output_array_ptr, &offset, bytes);

      output_array_position += bytes;
    }

    return unique_byte_array(output_buffer - sizeof(size_t), output_block_position + sizeof(size_t));
  }

  /**
   * Gets an upper bound on the size of the encoded buffer in bytes
   *
   * @param source_size The size of the unencoded buffer in bytes
   * @param bytes_per_block The number of bytes corresponding to one 
   * encoded block
   *
   * @return An upper bound on the size of the encoded buffer
   */
  static size_t get_buffer_size(size_t source_size) {
    int num_blocks = ceil(source_size / BYTES_PER_BLOCK);
    return sizeof(size_t) * num_blocks + LZ4_compressBound(std::max(source_size, BYTES_PER_BLOCK));
  }

 private:
  static size_t encode(char* buffer, char* source, size_t source_size) {
    return LZ4_compress_default(source, buffer, source_size, LZ4_compressBound(source_size));
  }

};

}
}

#endif /* CONFLUO_COMPRESSION_LZ4_ENCODER_H_ */

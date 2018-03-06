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

namespace confluo {

/**
 * A stateless LZ4 encoder. Takes as input a buffer of bytes
 * and number of bytes per block parameter and splits up the input data
 * into separate blocks that are each compressed using LZ4. These 
 * compressed blocks are fed into the decoder for partial or full decoding.
 */
class lz4_encoder {
 public:
  /**
   * Encodes the input buffer using LZ4 compression
   *
   * @param source_buffer The unecoded buffer to compress
   * @param source_length The size of the unencoded buffer in bytes
   * @param bytes_per_block The number of bytes corresponding to one
   * encoded block
   * @param output_buffer The output buffer containing the encoded data
   *
   * @return The size of the entire encoded buffer in bytes
   */
  static size_t encode(uint8_t* source_buffer, size_t source_length, uint8_t* output_buffer, size_t bytes_per_block = 65536) {

    size_t output_array_position = 0;

    int num_blocks = ceil(source_length / bytes_per_block);
    size_t output_block_position = sizeof(size_t) * num_blocks;

    for (size_t i = 0; i < source_length; i += bytes_per_block) {
      uint8_t* output_ptr = output_buffer + output_block_position;
      size_t size = encode((char*) output_ptr, (char*) (source_buffer + i), 
              bytes_per_block);
      output_block_position += size;

      uint8_t* output_array_ptr = output_buffer + output_array_position;
      size_t bytes = sizeof(size_t);
      size_t offset = output_block_position - size;
      std::memcpy(output_array_ptr, &offset, bytes);

      output_array_position += bytes;
    }

    return output_block_position;
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
  static size_t get_buffer_size(size_t source_size, size_t bytes_per_block = 65536) {
    int num_blocks = ceil(source_size / bytes_per_block);
    return sizeof(size_t) * num_blocks + LZ4_compressBound(source_size);
  }

 private:
  static size_t encode(char* buffer, char* source, size_t source_size) {
    return LZ4_compress_default(source, buffer, source_size, LZ4_compressBound(source_size));
  }

};

}

#endif /* CONFLUO_COMPRESSION_LZ4_ENCODER_H_ */

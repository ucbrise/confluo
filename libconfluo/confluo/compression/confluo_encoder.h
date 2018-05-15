#ifndef CONFLUO_COMPRESSION_CONFLUO_ENCODER_H_
#define CONFLUO_COMPRESSION_CONFLUO_ENCODER_H_

#include "delta_encoder.h"
#include "lz4_encoder.h"
#include "exceptions.h"
#include "storage/ptr_aux_block.h"
#include "storage/ptr_metadata.h"
#include "container/unique_byte_array.h"

namespace confluo {
namespace compression {

using namespace storage;

class confluo_encoder {
 public:

  /**
   * Encode pointer.
   * @param ptr unencoded data pointer, allocated by the storage allocator.
   * @param size size of unencoded data in bytes
   * @return pointer to raw encoded data
   */
  static unique_byte_array encode(void* ptr, size_t size, uint8_t encoding) {
    switch (encoding) {
      case encoding_type::D_UNENCODED: {
        return unique_byte_array(reinterpret_cast<uint8_t*>(ptr), size, no_op_delete);
      }
      case encoding_type::D_ELIAS_GAMMA: {
        uint64_t* casted = reinterpret_cast<uint64_t*>(ptr);
        size_t array_len = size / sizeof(uint64_t);
        return compression::delta_encoder::encode<uint64_t>(casted, array_len);
      }
      case encoding_type::D_LZ4: {
        uint8_t* casted = reinterpret_cast<uint8_t*>(ptr);
        return compression::lz4_encoder<>::encode(casted, size);
      }
      default : {
        THROW(illegal_state_exception, "Invalid encoding type!");
      }
    }
  }

 private:
  static void no_op_delete(uint8_t* ptr) { }

};

}
}

#endif /* CONFLUO_COMPRESSION_CONFLUO_ENCODER_H_ */

#include "compression/confluo_encoder.h"

namespace confluo {
namespace compression {

unique_byte_array confluo_encoder::encode(void *ptr, size_t size, uint8_t encoding) {
  switch (encoding) {
    case encoding_type::D_UNENCODED: {
      return unique_byte_array(reinterpret_cast<uint8_t *>(ptr), size, no_op_delete);
    }
    case encoding_type::D_ELIAS_GAMMA: {
      uint64_t *casted = reinterpret_cast<uint64_t *>(ptr);
      size_t array_len = size / sizeof(uint64_t);
      return delta_encoder::encode<uint64_t>(casted, array_len);
    }
    case encoding_type::D_LZ4: {
      uint8_t *casted = reinterpret_cast<uint8_t *>(ptr);
      return lz4_encoder<>::encode(casted, size);
    }
    default : {
      THROW(illegal_state_exception, "Invalid encoding type!");
    }
  }
}

void confluo_encoder::no_op_delete(uint8_t *) {}

}
}
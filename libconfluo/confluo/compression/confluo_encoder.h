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
  static unique_byte_array encode(void *ptr, size_t size, uint8_t encoding);

 private:
  /**
   * No-op deleter.
   *
   * @param ptr unencoded data pointer, allocated by the storage allocator.
   */
  static void no_op_delete(uint8_t *ptr);

};

}
}

#endif /* CONFLUO_COMPRESSION_CONFLUO_ENCODER_H_ */

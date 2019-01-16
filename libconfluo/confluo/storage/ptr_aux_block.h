#ifndef CONFLUO_STORAGE_PTR_AUX_BLOCK_H_
#define CONFLUO_STORAGE_PTR_AUX_BLOCK_H_

#include <cstdint>
#include "ptr_metadata.h"

namespace confluo {
namespace storage {

/**
 * Archival state of data pointed to.
 */
typedef struct state_type {
  static const uint8_t D_IN_MEMORY = 0;
  static const uint8_t D_ARCHIVED = 1;
} state_type;

/**
 * Encoding type of data pointed to.
 */
typedef struct encoding_type {
  static const uint8_t D_UNENCODED = 0;
  static const uint8_t D_LZ4 = 1;
  static const uint8_t D_ELIAS_GAMMA = 2;
} encoding_type;

/**
 * Pointer auxiliary block containing
 * contextual metadata about the data
 * pointed to. This data structure takes
 * 1B of space on its own but when set to
 * a ptr_metadata object will only use 4 bits.
 */
typedef struct ptr_aux_block {
  uint8_t encoding_ : 3;
  uint8_t state_ : 1;

  ptr_aux_block();

  ptr_aux_block(uint8_t state, uint8_t encoding);

  static ptr_aux_block get(ptr_metadata *metadata);

} aux_block;

}
}

#endif /* CONFLUO_STORAGE_PTR_AUX_BLOCK_H_ */

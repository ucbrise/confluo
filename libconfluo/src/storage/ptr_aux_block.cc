#include "storage/ptr_aux_block.h"

namespace confluo {
namespace storage {

const uint8_t state_type::D_IN_MEMORY;
const uint8_t state_type::D_ARCHIVED;
const uint8_t encoding_type::D_UNENCODED;

ptr_aux_block::ptr_aux_block()
    : ptr_aux_block(state_type::D_IN_MEMORY, encoding_type::D_UNENCODED) {
}

ptr_aux_block::ptr_aux_block(uint8_t state, uint8_t encoding)
    : encoding_(encoding),
      state_(state) {
}

ptr_aux_block ptr_aux_block::get(ptr_metadata *metadata) {
  uint8_t aux = metadata->aux_;
  return *reinterpret_cast<ptr_aux_block *>(&aux);
}

}
}
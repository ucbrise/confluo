#include "storage/ptr_metadata.h"

namespace confluo {
namespace storage {

const uint8_t alloc_type::D_DEFAULT;
const uint8_t alloc_type::D_MMAP;

ptr_metadata *ptr_metadata::get(void *ptr) {
  return static_cast<ptr_metadata *>(ptr) - 1;
}

}
}
#ifndef CONFLUO_STORAGE_PTR_METADATA_H_
#define CONFLUO_STORAGE_PTR_METADATA_H_

namespace confluo {
namespace storage {

struct alloc_type {
  static const uint8_t D_DEFAULT = 0;
  static const uint8_t D_MMAP = 1;
};

const uint8_t alloc_type::D_DEFAULT;
const uint8_t alloc_type::D_MMAP;

struct state_type {
  static const uint8_t D_IN_MEMORY = 0;
  static const uint8_t D_ARCHIVED = 1;
};

const uint8_t state_type::D_IN_MEMORY;
const uint8_t state_type::D_ARCHIVED;

typedef struct ptr_metadata {
  size_t size_ : 32;
  uint8_t thread_id_ : 8;
  uint8_t state_ : 4;
  uint8_t alloc_type_ : 4;
  int unused: 16;

  /**
   * Get metadata associated with a pointer
   * @param ptr pointer
   * @return metadata
   */
  static ptr_metadata* get(void* ptr) {
    return static_cast<ptr_metadata*>(ptr) - 1;
  }

} ptr_metadata;

}
}

#endif /* CONFLUO_STORAGE_PTR_METADATA_H_ */

#ifndef CONFLUO_STORAGE_PTR_METADATA_H_
#define CONFLUO_STORAGE_PTR_METADATA_H_

namespace confluo {
namespace storage {

/**
 * Type of allocation.
 */
struct alloc_type {
  /** Default allocation */
  static const uint8_t D_DEFAULT = 0;
  /** Memory mapped allocation */
  static const uint8_t D_MMAP = 1;
};

const uint8_t alloc_type::D_DEFAULT;
const uint8_t alloc_type::D_MMAP;

/**
 * State of data residing in allocated memory.
 */
struct state_type {
  /** In memory state */
  static const uint8_t D_IN_MEMORY = 0;
  /** Archived state */
  static const uint8_t D_ARCHIVED = 1;
};

const uint8_t state_type::D_IN_MEMORY;
const uint8_t state_type::D_ARCHIVED;

/**
 * Pointer metadata set for memory allocated by the
 * allocator. Not all fields may necessarily be set.
 */
typedef struct ptr_metadata {
  size_t data_size_ : 32; // size of data
  uint8_t thread_id_ : 8; // allocating thread id
  uint8_t state_ : 4; // data state
  uint8_t alloc_type_ : 4; // allocation type
  uint16_t offset_: 16; // offset from allocated pointer

  /**
   * Get metadata associated with a pointer
   * @param ptr The pointer to get metadaata from
   * @return The metadata associated with the pointer
   */
  static ptr_metadata* get(void* ptr) {
    return static_cast<ptr_metadata*>(ptr) - 1;
  }

} ptr_metadata;

}
}

#endif /* CONFLUO_STORAGE_PTR_METADATA_H_ */

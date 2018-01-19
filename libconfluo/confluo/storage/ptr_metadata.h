#ifndef CONFLUO_STORAGE_PTR_METADATA_H_
#define CONFLUO_STORAGE_PTR_METADATA_H_

namespace confluo {
namespace storage {

/**
 * The type of allocator
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
 * Type of state
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
 * Metadata from the pointer
 */
typedef struct ptr_metadata {
  /** Size of pointer */
  size_t size_ : 32;
  /** Identifier of the thread */
  uint8_t thread_id_ : 8;
  /** The state of the pointer */
  uint8_t state_ : 4;
  /** The allocation type */
  uint8_t alloc_type_ : 4;
  /** Amount unused */
  int unused: 16;

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

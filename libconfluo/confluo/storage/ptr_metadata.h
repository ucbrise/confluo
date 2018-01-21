#ifndef CONFLUO_STORAGE_PTR_METADATA_H_
#define CONFLUO_STORAGE_PTR_METADATA_H_

namespace confluo {
namespace storage {

// TODO add enums to convert between raw byte types

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
 * Archival state of data pointed to.
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
 * Encoding type of data pointed to.
 */
struct encoding_type {
  static const uint8_t D_UNENCODED = 0;
};

const uint8_t encoding_type::D_UNENCODED;

/**
 * Pointer metadata set for memory allocated by the
 * allocator. Not all fields may necessarily be set.
 */
typedef struct ptr_metadata {

  uint8_t alloc_type_ : 1; // allocation type
  uint16_t thread_id_ : 11; // allocating thread id
  uint16_t offset_: 16; // data offset from allocated pointer location
  size_t data_size_ : 32; // size of data
  uint8_t state_; // data-related state information

  /**
   * Get metadata associated with a pointer
   * @param ptr The pointer to get metadaata of
   * @return The metadata associated with the pointer
   */
  static ptr_metadata* get(void* ptr) {
    return static_cast<ptr_metadata*>(ptr) - 1;
  }

} ptr_metadata;

}
}

#endif /* CONFLUO_STORAGE_PTR_METADATA_H_ */

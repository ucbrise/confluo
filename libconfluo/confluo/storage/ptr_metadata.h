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
 * Pointer metadata set for memory allocated by the
 * allocator. Not all fields may necessarily be set.
 */
typedef struct ptr_metadata {

  // Do NOT re-order.
  uint32_t data_size_ : 32; // size of data
  uint16_t offset_: 16; // data offset from allocated pointer location
  uint16_t thread_id_ : 11; // allocating thread id
  uint8_t alloc_type_ : 1; // allocation type
  uint8_t aux_ : 4; // data-related state information

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

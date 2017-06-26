#ifndef DIALOG_POINTERS_H_
#define DIALOG_POINTERS_H_

namespace dialog {

/**
 * Block storage modes: compressed, and uncompressed.
 */
enum block_storage {
  UNCOMPRESSED = 0,//!< UNCOMPRESSED
  COMPRESSED = 1   //!< COMPRESSED
};

/**
 * Pointer to data on secondary storage.
 */
struct ss_ptr_t {
  uint8_t storage_type : 2;   // Storage type (UNCOMPRESSED/COMPRESSED)
  uint32_t file_id : 30;      // File ID: identifies the file data is stored in
  uint32_t file_off : 32;     // File offset where data is stored
};

/**
 * Fat pointer containing references to both data in memory, as well as data on
 * secondary storage.
 */
template<typename T>
struct fat_ptr_t {
  T* mem_ptr;                 // Pointer to data in memory
  ss_ptr_t ss_ptr;            // Pointer to data in secondary storage
};

}

#endif /* DIALOG_POINTERS_H_ */

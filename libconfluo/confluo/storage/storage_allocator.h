#ifndef CONFLUO_STORAGE_STORAGE_ALLOCATOR_H_
#define CONFLUO_STORAGE_STORAGE_ALLOCATOR_H_

#include <functional>
#include <unistd.h>

#include "conf/configuration_params.h"
#include "exceptions.h"
#include "memory_stat.h"
#include "mmap_utils.h"
#include "ptr_aux_block.h"
#include "ptr_metadata.h"

namespace confluo {
namespace storage {

// TODO: delegate requests to memory pools

/**
 * Storage allocator class. Interface for managing memory and storage
 */
class storage_allocator {
 public:
  typedef std::function<void(void)> callback_fn;

  /**
   * Initializes a storage allocator.
   */
  storage_allocator();

  /**
   * Register a cleanup callback
   * @param callback Callback function
   */
  void register_cleanup_callback(callback_fn callback);

  /**
   * Allocates memory and metadata for an object.
   * @param size size in bytes to allocate
   * @return pointer to allocated memory
   */
  void* alloc(size_t size, ptr_aux_block aux = ptr_aux_block());

  /**
   * Allocates new memory backed by file. Creates the file, overwriting old
   * data if the file already existed.
   *
   * @param path backing file
   * @param size size to allocate
   * @param state pointer state (bit field, constrained to storage::state_type)
   * @return pointer to memory
   */
  void* mmap(std::string path, size_t size, ptr_aux_block aux = ptr_aux_block());

  /**
   * Memory-maps part of an existing file.
   *
   * @param path path of file
   * @param offset file offset (does not need to be page aligned)
   * @param size size to mmap, exclusive of metadata
   * @param state pointer state (bit field, constrained to storage::state_type)
   * @return pointer to memory
   */
  void* mmap(std::string path, off_t offset, size_t size, ptr_aux_block aux = ptr_aux_block());

  /**
   * Deallocate or unmap pointer returned by this allocator.
   * @param ptr pointer to memory
   */
  void dealloc(void *ptr);

  /**
   * Gets the memory utilization stats
   *
   * @return The utilization of memory
   */
  size_t memory_utilization();

 private:
  memory_stat mem_stat_;
  memory_stat mmap_stat_;
  callback_fn mem_cleanup_callback_;

  static const int MAX_CLEANUP_RETRIES = 10;
  static void no_op() {}
};

}
}

#endif /* CONFLUO_STORAGE_STORAGE_ALLOCATOR_H_ */

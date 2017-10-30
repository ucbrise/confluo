#ifndef CONFLUO_STORAGE_STORAGE_ALLOCATOR_H_
#define CONFLUO_STORAGE_STORAGE_ALLOCATOR_H_

#include <unistd.h>

#include "exceptions.h"
#include "mmap_utils.h"
#include "storage/memory_stat.h"
#include "storage/ptr_metadata.h"

namespace confluo {
namespace storage {

using namespace ::utils;
// TODO: delegate requests to memory pools

/**
 * Storage allocator class. Interface for managing memory and storage
 */
class storage_allocator {

 public:
  /**
   * Initializes a storage allocator
   */
  storage_allocator() :
    mem_stat_(),
    mmap_stat_() {
  }

  /**
   * Allocates memory and metadata for len instances of T
   * @param len length of array to allocate
   * @return pointer to allocated memory
   */
  template<typename T>
  T* alloc(size_t len = 1) {
    if (mem_stat_.get() >= configuration_params::MAX_MEMORY) {
      THROW(memory_exception, "Max memory reached!");
    }
    size_t alloc_size = sizeof(ptr_metadata) + len * sizeof(T);
    mem_stat_.increment(alloc_size);

    // allocate contiguous memory for both the ptr and metadata
    void* ptr = ::operator new(alloc_size);
    ptr_metadata* md = new (ptr) ptr_metadata;
    T* data_ptr = reinterpret_cast<T*>(md + 1);

    md->alloc_type_ = alloc_type::D_DEFAULT;
    md->state_ = state_type::D_IN_MEMORY;
    md->size_ = len * sizeof(T);

    return data_ptr;
  }

  /**
   * Allocates new memory backed by file. Creates the file, overwriting old
   * data if the file already existed.
   *
   * @param path backing file.
   * @param len length of array
   * @param state pointer state (bit field, constrained to storage::state_type)
   * @return pointer to memory
   */
  template<typename T>
  T* mmap(std::string path, size_t len = 1, uint8_t state = state_type::D_IN_MEMORY) {
    size_t alloc_size = sizeof(ptr_metadata) + len * sizeof(T);
    mmap_stat_.increment(alloc_size);

    int fd = utils::file_utils::open_file(path, O_CREAT | O_TRUNC | O_RDWR);
    file_utils::truncate_file(fd, alloc_size);
    void* ptr = mmap_utils::map(fd, nullptr, 0, alloc_size);
    file_utils::close_file(fd);

    storage::ptr_metadata* metadata = static_cast<ptr_metadata*>(ptr);
    metadata->alloc_type_ = alloc_type::D_MMAP;
    metadata->state_ = state;
    metadata->size_ = len * sizeof(T);

    return reinterpret_cast<T*>(metadata + 1);
  }

  /**
   * Memory-maps part of an existing file.
   *
   * @param path path of file
   * @param offset file offset (does not need to be page aligned)
   * @param len length of array
   * @param state pointer state (bit field, constrained to storage::state_type)
   * @return pointer to memory
   */
  template<typename T>
  T* mmap(std::string path, off_t offset, size_t len, uint8_t state) {
    off_t page_aligned_offset = offset - (offset % getpagesize());
    int mmap_delta = offset - page_aligned_offset;

    size_t mmap_size = sizeof(ptr_metadata) + len * sizeof(T) + mmap_delta;
    mmap_stat_.increment(mmap_size);

    int fd = file_utils::open_file(path, O_RDWR);
    uint8_t* ptr = static_cast<uint8_t*>(mmap_utils::map(fd, nullptr, page_aligned_offset, mmap_size));
    ptr += mmap_delta;
    file_utils::close_file(fd);

    storage::ptr_metadata* metadata = reinterpret_cast<ptr_metadata*>(ptr);
    metadata->alloc_type_ = alloc_type::D_MMAP;
    metadata->state_ = state;
    metadata->size_ = len * sizeof(T);
    metadata->offset_ = mmap_delta;

    return reinterpret_cast<T*>(metadata + 1);
  }

  /**
   * Deallocate memory allocated by this allocator
   * @param ptr pointer to allocated memory
   */
  template<typename T>
  void dealloc(T* ptr) {
    ptr_metadata* md = ptr_metadata::get(ptr);
    size_t alloc_size = sizeof(ptr_metadata) + md->size_;
    switch (md->alloc_type_) {
    case alloc_type::D_DEFAULT:
      md->~ptr_metadata();
      ::operator delete(static_cast<void*>(md));
      mem_stat_.decrement(alloc_size);
      break;
    case alloc_type::D_MMAP:
      size_t total_offset = ptr_metadata::get(ptr)->offset_ + sizeof(ptr_metadata);
      mmap_utils::unmap(reinterpret_cast<uint8_t*>(ptr) - total_offset, alloc_size);
      mmap_stat_.decrement(alloc_size);
      break;
    }
  }

  /**
   * Gets the memory utilization stats
   *
   * @return The utilization of memory
   */
  size_t memory_utilization() {
    return mem_stat_.get();
  }

 private:
  memory_stat mem_stat_;
  memory_stat mmap_stat_;

};

}
}

#endif /* CONFLUO_STORAGE_STORAGE_ALLOCATOR_H_ */

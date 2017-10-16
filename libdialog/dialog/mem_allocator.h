#ifndef DIALOG_DIALOG_ALLOCATOR_H_
#define DIALOG_DIALOG_ALLOCATOR_H_

#include "exceptions.h"
#include "memory_stat.h"
#include "mmap_utils.h"
#include "ptr_metadata.h"

namespace dialog {
namespace memory {

using namespace ::utils;

/**
 * Allocator interface
 * TODO: delegate requests to memory pools
 */
class mem_allocator {

 public:
  mem_allocator() :
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
    for (T* p = data_ptr; p < data_ptr + len; p++) {
      new (p) T;
    }

    md->alloc_type_ = alloc_type::D_DEFAULT;
    md->state_ = state_type::D_IN_MEMORY;
    md->size_ = len * sizeof(T);

    return data_ptr;
  }

  /**
   * Allocates new memory backed by file. Creates the file, overwriting old
   * data if the file already existed.
   *
   * @param path Backing file.
   * @param len Length of array
   * @return pointer to memory
   */
  template<typename T>
  T* mmap(std::string path, size_t len = 1) {
    size_t alloc_size = sizeof(ptr_metadata) + len * sizeof(T);
    mmap_stat_.increment(alloc_size);

    int fd = utils::file_utils::create_file(path, O_CREAT | O_TRUNC | O_RDWR);
    file_utils::truncate_file(fd, alloc_size);
    void* data = mmap_utils::map(fd, nullptr, 0, alloc_size);
    file_utils::close_file(fd);

    memory::ptr_metadata* metadata = static_cast<ptr_metadata*>(data);
    metadata->alloc_type_ = memory::alloc_type::D_MMAP;
    metadata->state_ = memory::state_type::D_IN_MEMORY;
    metadata->size_ = len * sizeof(T);

    return reinterpret_cast<T*>(metadata + 1);
  }

  /**
   * Deallocate memory allocated by this allocator
   * @param ptr pointer to allocated memory
   */
  template<typename T>
  void dealloc(T* ptr) {
    ptr_metadata* md = ptr_metadata::get_metadata(ptr);
    size_t alloc_size = sizeof(ptr_metadata) + md->size_;
    switch (md->alloc_type_) {
    case D_DEFAULT:
      md->~ptr_metadata();
      for (T* p = ptr; p < ptr + md->size_/sizeof(T); p++) {
        p->~T();
      }
      ::operator delete(static_cast<void*>(md));
      mem_stat_.decrement(alloc_size);
      break;
    case D_MMAP:
      mmap_utils::unmap(reinterpret_cast<ptr_metadata*>(ptr) - 1, alloc_size);
      mmap_stat_.decrement(alloc_size);
      break;
    }
  }

  size_t memory_utilization() {
    return mem_stat_.get();
  }

 private:
  memory_stat mem_stat_;
  memory_stat mmap_stat_;

};

}
}

#endif /* DIALOG_DIALOG_ALLOCATOR_H_ */

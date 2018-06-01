#include "storage/storage_allocator.h"

namespace confluo {
namespace storage {

const int storage_allocator::MAX_CLEANUP_RETRIES;

storage_allocator::storage_allocator()
    : mem_stat_(),
      mmap_stat_(),
      mem_cleanup_callback_(no_op) {
}

void storage_allocator::register_cleanup_callback(storage_allocator::callback_fn callback) {
  mem_cleanup_callback_ = callback;
}

void *storage_allocator::alloc(size_t size, ptr_aux_block aux) {
  int retries = 0;
  while (mem_stat_.get() >= configuration_params::MAX_MEMORY()) {
    mem_cleanup_callback_();
    if (retries > MAX_CLEANUP_RETRIES)
      THROW(memory_exception, "Max memory reached!");
    retries++;
  }
  size_t alloc_size = sizeof(ptr_metadata) + size;
  mem_stat_.increment(alloc_size);

  // allocate contiguous memory for both the ptr and metadata
  void* ptr = malloc(alloc_size);
  ptr_metadata* md = new (ptr) ptr_metadata;
  void* data_ptr = reinterpret_cast<void*>(md + 1);

  md->alloc_type_ = alloc_type::D_DEFAULT;
  md->data_size_ = size;
  md->offset_ = 0;
  md->aux_ = *reinterpret_cast<uint8_t*>(&aux);

  return data_ptr;
}

void *storage_allocator::mmap(std::string path, size_t size, ptr_aux_block aux) {
  size_t alloc_size = sizeof(ptr_metadata) + size;
  mmap_stat_.increment(alloc_size);

  int fd = utils::file_utils::open_file(path, O_CREAT | O_TRUNC | O_RDWR);
  utils::file_utils::truncate_file(fd, alloc_size);
  void *ptr = utils::mmap_utils::map(fd, nullptr, 0, alloc_size);
  utils::file_utils::close_file(fd);

  storage::ptr_metadata *metadata = static_cast<ptr_metadata *>(ptr);
  metadata->alloc_type_ = alloc_type::D_MMAP;
  metadata->data_size_ = static_cast<uint32_t>(size);
  metadata->offset_ = 0;
  metadata->aux_ = *reinterpret_cast<uint8_t *>(&aux);

  return reinterpret_cast<void *>(metadata + 1);
}

void *storage_allocator::mmap(std::string path, off_t offset, size_t size, ptr_aux_block aux) {
  int mmap_delta = static_cast<int>(offset % getpagesize());
  off_t page_aligned_offset = offset - mmap_delta;

  size_t mmap_size = sizeof(ptr_metadata) + size + mmap_delta;
  mmap_stat_.increment(mmap_size);

  int fd = utils::file_utils::open_file(path, O_RDWR);
  uint8_t *ptr =
      static_cast<uint8_t *>(utils::mmap_utils::map(fd, nullptr, static_cast<size_t>(page_aligned_offset), mmap_size));
  utils::file_utils::close_file(fd);

  ptr += mmap_delta;

  storage::ptr_metadata *metadata = reinterpret_cast<ptr_metadata *>(ptr);
  metadata->alloc_type_ = alloc_type::D_MMAP;
  metadata->data_size_ = static_cast<uint32_t>(size);
  metadata->offset_ = static_cast<uint16_t>(mmap_delta);
  metadata->aux_ = *reinterpret_cast<uint8_t *>(&aux);

  return reinterpret_cast<void *>(metadata + 1);
}

void storage_allocator::dealloc(void *ptr) {
  ptr_metadata *md = ptr_metadata::get(ptr);
  size_t alloc_size = sizeof(ptr_metadata) + md->data_size_ + md->offset_;
  switch (md->alloc_type_) {
    case alloc_type::D_DEFAULT: {
      md->~ptr_metadata();
      free(md);
      mem_stat_.decrement(alloc_size);
      break;
    }
    case alloc_type::D_MMAP: {
      size_t total_offset = ptr_metadata::get(ptr)->offset_ + sizeof(ptr_metadata);
      utils::mmap_utils::unmap(reinterpret_cast<uint8_t *>(ptr) - total_offset, alloc_size);
      mmap_stat_.decrement(alloc_size);
      break;
    }
    default: {
      throw invalid_operation_exception("Invalid allocation type");
    }
  }
}

size_t storage_allocator::memory_utilization() {
  return mem_stat_.get();
}

}
}
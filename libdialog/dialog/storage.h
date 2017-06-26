#ifndef DIALOG_STORAGE_H_
#define DIALOG_STORAGE_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include "file_utils.h"
#include "mmap_utils.h"
#include "assertions.h"

#define PROT_RW PROT_READ | PROT_WRITE

namespace dialog {
namespace storage {

// TODO: Add documentation
// TODO: Improve allocation using pools and file consolidation.

using namespace ::utils;

/**
 * In memory storage, uses mmap to allocate data that is not backed by a file.
 */
struct in_memory {
  /**
   * Allocates new memory.
   *
   * @param path Backing file (unused).
   * @param size Size of requested memory.
   */
  inline static void* allocate(const std::string& path, size_t size) {
    return mmap_utils::map(-1, nullptr, 0, size, PROT_RW,
                           MAP_SHARED | MAP_ANONYMOUS);
  }

  /**
   * Frees allocated memory.
   *
   * @param ptr Pointer to memory to be freed.
   * @param size Size of allocated memory.
   */
  inline static void free(void* ptr, size_t size) {
    return mmap_utils::unmap(ptr, size);
  }

  /**
   * Flushes data to backed file (does nothing for this storage mode).
   *
   * @param ptr Pointer to memory.
   * @param size Size of allocated memory.
   */
  inline static void flush(void* ptr, size_t size) {
    return;
  }
};

/**
 * Durable (relaxed) storage mode, which allocates memory backed by a file, but
 * delegates flushing of the data to the file to the OS kernel.
 */
struct durable_relaxed {
  /**
   * Allocates new memory backed by file. Creates the file, overwriting old
   * data if the file already existed.
   *
   * @param path Backing file.
   * @param size Size of requested memory.
   */
  inline static void* allocate(const std::string& path, size_t size) {
    int fd = utils::file_utils::create_file(path, O_CREAT | O_TRUNC | O_RDWR);
    file_utils::truncate_file(fd, size);
    void* data = mmap_utils::map(fd, nullptr, 0, size);
    file_utils::close_file(fd);
    return data;
  }

  /**
   * Frees allocated memory. Does not delete backing file.
   *
   * @param ptr Pointer to memory to be freed.
   * @param size Size of allocated memory.
   */
  inline static void free(void* ptr, size_t size) {
    mmap_utils::unmap(ptr, size);
  }

  /**
   * Flushes data to backed file (does nothing for this storage mode).
   *
   * @param ptr Pointer to memory.
   * @param size Size of allocated memory.
   */
  inline static void flush(void* ptr, size_t size) {
    return;
  }
};

/**
 * Durable (relaxed) storage mode, which allocates memory backed by a file, and
 * also supports manually flushing of the data to the file.
 */
struct durable {
  /**
   * Allocates new memory backed by file. Creates the file, overwriting old
   * data if the file already existed.
   *
   * @param path Backing file.
   * @param size Size of requested memory.
   */
  inline static void* allocate(const std::string& path, size_t size) {
    int fd = utils::file_utils::create_file(path, O_CREAT | O_TRUNC | O_RDWR);
    file_utils::truncate_file(fd, size);
    void* data = mmap_utils::map(fd, nullptr, 0, size);
    file_utils::close_file(fd);
    return data;
  }

  /**
   * Frees allocated memory. Does not delete backing file.
   *
   * @param ptr Pointer to memory to be freed.
   * @param size Size of allocated memory.
   */
  inline static void free(void* ptr, size_t size) {
    mmap_utils::unmap(ptr, size);
  }

  /**
   * Flushes data to backed file (does nothing for this storage mode).
   *
   * @param ptr Pointer to memory.
   * @param size Size of allocated memory.
   */
  inline static void flush(void* ptr, size_t size) {
    mmap_utils::flush(ptr, size);
  }
};

}
}

#endif /* DIALOG_STORAGE_H_ */

#ifndef CONFLUO_STORAGE_STORAGE_H_
#define CONFLUO_STORAGE_STORAGE_H_

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

#include "assertions.h"
#include "file_utils.h"
#include "mmap_utils.h"
#include "storage/ptr.h"
#include "storage/ptr_metadata.h"
#include "storage_allocator.h"

#define PROT_RW PROT_READ | PROT_WRITE

namespace confluo {
namespace storage {

// TODO: Add documentation
// TODO: Improve allocation using pools and file consolidation.

using namespace ::utils;

/** Allocate function that allocates a file of a certain size */
typedef void* (*allocate_fn)(const std::string& path, size_t size);
/** Allocates a block in a file of a certain size */
typedef uint8_t* (*allocate_block_fn)(const std::string& path, size_t size);
/** Frees the given pointer */
typedef void (*free_fn)(void* ptr, size_t size);
/** Flushes the memory specified by the pointer */
typedef void (*flush_fn)(void* ptr, size_t size);

/**
 * Contains the particular storage mode
 */
enum storage_mode {
  /** Stores data in memory */
  IN_MEMORY = 0,
  /** Has relaxed linearizable guarantees */
  DURABLE_RELAXED = 1,
  /** Persisted storage */
  DURABLE = 2
};

/**
 * Functionality for a storage function
 */
struct storage_functions {
  /** The particular storage mode */
  storage_mode mode;
  /** The allocation function */
  allocate_fn allocate;
  /** The function that allocates a block of memory */
  allocate_block_fn allocate_block;
  /** Function that frees memory */
  free_fn free;
  /** Function that flushes memory */
  flush_fn flush;
};

/**
 * In memory storage, uses malloc or mempool to allocate data that is not backed by a file.
 */
struct in_memory {
  /**
   * Allocates new memory.
   *
   * @param path Backing file (unused).
   * @param size Size of requested memory.
   * @return Allocated bytes.
   */
  inline static void* allocate(const std::string& path, size_t size) {
    return malloc(size);
  }

  /**
   * Allocates a block of new memory.
   *
   * @param path Backing file (unused).
   * @param size Size of requested block.
   * @return Allocated block.
   */
  inline static uint8_t* allocate_block(const std::string& path, size_t size) {
    return ALLOCATOR.alloc<uint8_t>(size / sizeof(uint8_t));
  }

  /**
   * Frees allocated memory.
   *
   * @param ptr Pointer to memory to be freed.
   * @param size Size of allocated memory.
   */
  inline static void free_mem(void* ptr, size_t size) {
    free(ptr);
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
   * @return Allocated bytes.
   */
  inline static void* allocate(const std::string& path, size_t size) {
    int fd = utils::file_utils::open_file(path, O_CREAT | O_TRUNC | O_RDWR);
    file_utils::truncate_file(fd, size);
    void* data = mmap_utils::map(fd, nullptr, 0, size);
    file_utils::close_file(fd);
    return data;
  }

  /**
   * Allocates a block of new memory.
   *
   * @param path Backing file.
   * @param size Size of requested block.
   * @return Allocated block.
   */
  inline static uint8_t* allocate_block(const std::string& path, size_t size) {
    return ALLOCATOR.mmap<uint8_t>(path, size / sizeof(uint8_t));
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
   * @return Allocated bytes.
   */
  inline static void* allocate(const std::string& path, size_t size) {
    int fd = utils::file_utils::open_file(path, O_CREAT | O_TRUNC | O_RDWR);
    file_utils::truncate_file(fd, size);
    void* data = mmap_utils::map(fd, nullptr, 0, size);
    file_utils::close_file(fd);
    return data;
  }

  /**
   * Allocates a block of new memory.
   *
   * @param path Backing file.
   * @param size Size of requested block.
   * @return Allocated block.
   */
  inline static uint8_t* allocate_block(const std::string& path, size_t size) {
    return ALLOCATOR.mmap<uint8_t>(path, size / sizeof(uint8_t));
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
   * TODO: add method for swappable_ptr
   * @param ptr Pointer to memory.
   * @param size Size of allocated memory.
   */
  inline static void flush(void* ptr, size_t size) {
    mmap_utils::flush(ptr, size);
  }
};

/** Storage functionality for in memory mode */
static storage_functions IN_MEMORY_FNS = { storage_mode::IN_MEMORY,
    in_memory::allocate, in_memory::allocate_block, in_memory::free_mem,
    in_memory::flush };

/** Storage functionality for durable relaxed mode */
static storage_functions DURABLE_RELAXED_FNS = { storage_mode::DURABLE_RELAXED,
    durable_relaxed::allocate, durable_relaxed::allocate_block,
    durable_relaxed::free, durable_relaxed::flush };

/** Storage functionality for durable mode */
static storage_functions DURABLE_FNS = { storage_mode::DURABLE,
    durable::allocate, durable::allocate_block, durable::free, durable::flush };

/** Contains the storage functions for all storage modes */
static storage_functions STORAGE_FNS[3] = { IN_MEMORY_FNS, DURABLE_RELAXED_FNS,
    DURABLE_FNS };

}
}

#endif /* CONFLUO_STORAGE_STORAGE_H_ */

#ifndef CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_BLOCK_H_
#define CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_BLOCK_H_

#include "atomic.h"
#include "storage/ptr.h"
#include "storage/storage.h"
#include "io_utils.h"

namespace confluo {
namespace monolog {

using namespace ::utils;

/**
 * Block in a monolog
 *
 * @tparam T
 * @tparam BUFFER_SIZE
 */
template<typename T, size_t BUFFER_SIZE = 1048576>
class monolog_block {
 public:
  typedef storage::swappable_ptr<T> __atomic_block_ref;
  typedef storage::read_only_ptr<T> __atomic_block_copy_ref;

  typedef bool block_state;
  static const block_state UNINIT = false;
  static const block_state INIT = true;

  /**
   * monolog_block
   */
  monolog_block()
      : path_(""),
        state_(UNINIT),
        data_(),
        size_(0),
        mode_(storage::IN_MEMORY) {
  }

  /**
   * monolog_block
   *
   * @param path The path
   * @param size The size
   * @param storage The storage
   */
  monolog_block(const std::string& path, size_t size,
                const storage::storage_mode& storage)
      : path_(path),
        state_(UNINIT),
        data_(nullptr),
        size_(size),
        mode_(storage) {
  }

  /**
   * monolog_block
   *
   * @param other The other
   */
  monolog_block(const monolog_block<T, BUFFER_SIZE>& other)
      : path_(other.path_),
        state_(other.state_),
        size_(other.size_),
        mode_(other.mode_) {
    __atomic_block_copy_ref copy;
    other.data_.atomic_copy(copy);
    data_.atomic_init(copy.ptr_);
  }

  /**
   * init
   *
   * @param path The path
   * @param size The size
   * @param mode The mode
   */
  void init(const std::string& path, const size_t size,
            const storage::storage_mode& mode) {
    path_ = path;
    size_ = size;
    mode_ = mode;
  }

  /**
   * storage_size
   *
   * @return size_t
   */
  size_t storage_size() const {
    if (data_.atomic_load() != nullptr)
      return (size_ + BUFFER_SIZE) * sizeof(T);
    return 0;
  }

  void flush(size_t offset, size_t len) {
    __atomic_block_copy_ref copy;
    data_.atomic_copy(copy);
    storage::STORAGE_FNS[mode_].flush(copy.get() + offset, len * sizeof(T));
  }

  /**
   * Sets value at index. Allocates bucket if necessary.
   * Assumes no contention between writers and archiver.
   * @param i index
   * @param val value
   */
  void set(size_t i, const T& val) {
    T* ptr = data_.atomic_load();
    if (ptr == nullptr) {
      ptr = try_allocate();
    }
    ptr[i] = val;
  }

  /**
   * Sets value at index. Assumes no contention between
   * writers and archiver.
   * @param i index
   * @param val value
   */
  void set_unsafe(size_t i, const T& val) {
    data_.atomic_load()[i] = val;
  }

  /**
   * Writes data to offset. Allocates bucket if necessary.
   * Assumes no contention between writers and archiver.
   * @param offset offset to write at
   * @param data data to write
   * @param len length of data
   */
  void write(size_t offset, const T* data, size_t len) {
    T* ptr = data_.atomic_load();
    if (ptr == nullptr) {
      ptr = try_allocate();
    }
    memcpy(ptr + offset, data, len * sizeof(T));
  }

  /**
   * Writes data to offset. Assumes no contention between
   * writers and archiver.
   * @param offset offset to write at
   * @param data data to write
   * @param len length of data
   */
  void write_unsafe(size_t offset, const T* data, size_t len) {
    memcpy(data_.atomic_load() + offset, data, len * sizeof(T));
  }

  /**
   * at
   *
   * @param i The i
   *
   * @return T
   */
  const T at(size_t i) const {
    return data_.atomic_get(i);
  }

  /**
   * read
   *
   * @param offset The offset
   * @param data The data
   * @param len The len
   */
  void read(size_t offset, T* data, size_t len) const {
    memcpy(data, data_.atomic_copy()->get() + offset, len * sizeof(T));
  }

  /**
   * operator[]
   *
   * @param i The i
   *
   * @return T&
   */
  T& operator[](size_t i) {
    __atomic_block_copy_ref copy;
    data_.atomic_copy(copy);
    if (copy.get() == nullptr)
      try_allocate(copy);
    return copy.get()[i];
  }

  /**
   * ptr
   *
   * @param offset The offset
   * @param data_ptr The data_ptr
   */
  void ptr(size_t offset, __atomic_block_copy_ref& data_ptr) {
    data_.atomic_copy(data_ptr, offset);
    if (data_ptr.get() == nullptr) {
      try_allocate(data_ptr);
      data_ptr.set_offset(offset);
    }
  }

  /**
   * cptr
   *
   * @param offset The offset
   * @param data_ptr The data_ptr
   */
  void cptr(size_t offset, __atomic_block_copy_ref& data_ptr) const {
    data_.atomic_copy(data_ptr, offset);
  }

  /**
   * operator=
   *
   * @param other The other
   *
   * @return monolog_block&
   */
  monolog_block& operator=(const monolog_block<T, BUFFER_SIZE>& other) {
    path_ = other.path_;
    atomic::init(&state_, atomic::load(&other.state_));
    __atomic_block_copy_ref copy;
    other.data_.atomic_copy(copy);
    data_.atomic_init(copy.ptr_);
    return *this;
  }

  /**
   * ensure_alloc
   */
  void ensure_alloc() {
    if (data_.atomic_load() == nullptr) {
      __atomic_block_copy_ref copy;
      try_allocate(copy);
    }
  }

  /**
   * Swap current pointer with new pointer.
   * @param ptr new pointer
   */
  void swap_ptr(T* ptr) {
    data_.swap_ptr(ptr);
  }

private:
  void try_allocate(__atomic_block_copy_ref& copy) {
    block_state state = UNINIT;
    if (atomic::strong::cas(&state_, &state, INIT)) {
      size_t file_size = (size_ + BUFFER_SIZE) * sizeof(T);
      T* ptr = storage::STORAGE_FNS[mode_].allocate_block(path_, file_size);
      memset(ptr, '\0', sizeof(T) * file_size);
      data_.atomic_init(ptr);
      data_.atomic_copy(copy);
      return;
    }

    // Someone else is initializing, stall until initialized
    while (data_.atomic_load() == nullptr)
    ;

    data_.atomic_copy(copy);
  }

  T* try_allocate() {
    block_state state = UNINIT;
    if (atomic::strong::cas(&state_, &state, INIT)) {
      size_t file_size = (size_ + BUFFER_SIZE) * sizeof(T);
      T* ptr = storage::STORAGE_FNS[mode_].allocate_block(path_, file_size);
      memset(ptr, '\0', sizeof(T) * file_size);
      data_.atomic_init(ptr);
      return ptr;
    }

    // Someone else is initializing, stall until initialized
    T* ptr;
    while ((ptr = data_.atomic_load()) == nullptr)
    ;
    return ptr;
  }

  std::string path_;
  atomic::type<block_state> state_;
  __atomic_block_ref data_;
  size_t size_;
  storage::storage_mode mode_;
};

template<typename T, size_t BUFFER_SIZE>
const bool monolog_block<T, BUFFER_SIZE>::INIT;

template<typename T, size_t BUFFER_SIZE>
const bool monolog_block<T, BUFFER_SIZE>::UNINIT;

}
}

#endif /* CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_BLOCK_H_ */

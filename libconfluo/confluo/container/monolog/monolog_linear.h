#ifndef CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_
#define CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_

#include <array>
#include <vector>

#include "atomic.h"
#include "monolog_linear_block.h"
#include "storage/ptr.h"
#include "storage/storage.h"

namespace confluo {
namespace monolog {

/**
 * Monolog linear base class. The monolog grow linearly, block-by-block, as
 * space runs out in previous blocks.
 */
template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 268435456,
    size_t BUFFER_SIZE = 1048576>
class monolog_linear_base {
 public:
  monolog_linear_base() = default;

  /**
   * Constructor to initialize monolog with specified name, data path and
   * storage mode.
   *
   * @param name The name of the monolog.
   * @param data_path The data path where the monolog is stored.
   * @param storage The storage mode for the monolog.
   */
  monolog_linear_base(const std::string& name, const std::string& data_path,
                      const storage::storage_mode& storage) {
    init(name, data_path, storage);
  }

  /**
   * Initialize the monolog with specified name, data path and storage mode.
   *
   * @param name The name of the monolog.
   * @param data_path The data path where the monolog is stored.
   * @param storage The storage mode for the monolog.
   */
  void init(const std::string& name, const std::string& data_path,
            const storage::storage_mode& storage) {
    name_ = name;
    data_path_ = data_path;
    for (size_t i = 0; i < MAX_BLOCKS; i++) {
      std::string block_path = data_path + "/" + name + "_" + std::to_string(i)
          + ".dat";
      blocks_[i].init(block_path, BLOCK_SIZE, storage);
    }
    blocks_[0].ensure_alloc();
  }

  /**
   * Get the name of the monolog.
   *
   * @return The name of the monolog.
   */
  std::string name() const {
    return name_;
  }

  /**
   * Get the data path for the monolog.
   *
   * @return The data path for the monolog.
   */
  std::string data_path() const {
    return data_path_;
  }

  /**
   * Ensures containers are allocated to cover the range of indexes given.
   * @param start_idx start index
   * @param end_idx end index
   */
  void ensure_alloc(size_t start_idx, size_t end_idx) {
    size_t bucket_idx1 = start_idx / BLOCK_SIZE;
    size_t bucket_idx2 = end_idx / BLOCK_SIZE;
    for (size_t i = bucket_idx1; i <= bucket_idx2; i++) {
      blocks_[i].ensure_alloc();
    }
  }

  /**
   * Sets the data at index idx to val. Allocates memory if necessary.
   * @param idx index to set at
   * @param val value to set
   */
  void set(size_t idx, const T& val) {
    blocks_[idx / BLOCK_SIZE].set(idx % BLOCK_SIZE, val);
  }

  /**
   * Sets the data at index idx to val. Does NOT allocate memory --
   * ensure memory is allocated before calling this function.
   * @param idx index to set at
   * @param val value to set
   */
  void set_unsafe(size_t idx, const T val) {
    blocks_[idx / BLOCK_SIZE].set_unsafe(idx % BLOCK_SIZE, val);
  }

  /**
   * Write len bytes of data at idx. Allocates memory if necessary.
   * @param idx monolog index
   * @param data data to write
   * @param len length of data
   */
  void write(size_t idx, const T* data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = idx / BLOCK_SIZE;
      size_t bucket_off = idx % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      blocks_[bucket_idx].write(bucket_off, data + len - remaining, bucket_len);
      idx += bucket_len;
      remaining -= bucket_len;
    }
  }

  /**
   * Write len bytes of data at idx. Does NOT allocate memory -- ensure
   * memory is allocated before calling this function.
   * @param idx monolog index
   * @param data data to write
   * @param len length of data
   */
  void write_unsafe(size_t idx, const T* data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = idx / BLOCK_SIZE;
      size_t bucket_off = idx % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      blocks_[bucket_idx].write_unsafe(bucket_off, data + len - remaining,
                                       bucket_len);

      idx += bucket_len;
      remaining -= bucket_len;
    }
  }

  /**
   * Flush data at idx
   * @param idx monolog index
   * @param len length of data to flush
   */
  void flush(size_t idx, size_t len) {
    blocks_[idx / BLOCK_SIZE].flush(idx % BLOCK_SIZE, len);
  }

  /**
   * Gets data at idx
   * @param idx monolog index
   * @return data
   */
  const T get(size_t idx) const {
    return blocks_[idx / BLOCK_SIZE].at(idx % BLOCK_SIZE);
  }

  /**
   * Read a specified number of elements from monolog at a specified offset.
   * @param offset The offset to read from.
   * @param data The buffer that the data will be read into.
   * @param len The number of bytes to read.
   */
  void read(size_t offset, T* data, size_t len) const {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BLOCK_SIZE;
      size_t bucket_off = offset % BLOCK_SIZE;
      size_t bucket_len = std::min(BLOCK_SIZE - bucket_off, remaining);
      blocks_[bucket_idx].read(bucket_off, data + len - remaining, bucket_len);
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  /**
   * Gets a pointer to the data at specified index
   * @param idx monolog index
   * @param data_ptr read-only pointer to store in
   */
  void ptr(size_t idx, storage::read_only_ptr<T>& data_ptr) {
    blocks_[idx / BLOCK_SIZE].ptr(idx % BLOCK_SIZE, data_ptr);
  }

  /**
   * Gets a pointer to the data at specified index
   * @param idx monolog index
   * @param data_ptr read-only pointer to store in
   */
  void cptr(size_t idx, storage::read_only_ptr<T>& data_ptr) const {
    blocks_[idx / BLOCK_SIZE].cptr(idx % BLOCK_SIZE, data_ptr);
  }

  /**
   * operator[] (reference)
   * @param idx Index of reference to get.
   * @return Reference to requested data.
   */
  T& operator[](size_t idx) {
    return blocks_[idx / BLOCK_SIZE][idx % BLOCK_SIZE];
  }

  /**
   * Swaps the block pointer with given pointer.
   *
   * @param idx The index of the data.
   * @param ptr The pointer to swap the block pointer with.
   */
  void swap_block_ptr(size_t idx, T* ptr) {
    blocks_[idx / BLOCK_SIZE].swap_ptr(ptr);
  }

  /**
   * Get the storage size of the monolog.
   *
   * @return storage size of the monolog.
   */
  size_t storage_size() const {
    size_t bucket_size = blocks_.size() * sizeof(monolog_block<T, BLOCK_SIZE> );
    size_t data_size = 0;
    for (size_t i = 0; i < blocks_.size(); i++)
      data_size += blocks_[i].storage_size();
    return bucket_size + data_size;
  }

 protected:
  std::string name_;
  std::string data_path_;
  std::array<monolog_block<T, BUFFER_SIZE>, MAX_BLOCKS> blocks_;

 private:

};

/**
 * Monolog linear class. The monolog grow linearly, block-by-block, as
 * space runs out in previous blocks.
 */
template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 268435456,
    size_t BUFFER_SIZE = 1048576>
class monolog_linear : public monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE,
    BUFFER_SIZE> {
 public:
  // Type definitions
  typedef size_t size_type;
  typedef size_t pos_type;
  typedef T value_type;
  typedef T difference_type;
  typedef T* pointer;
  typedef T reference;
  typedef monolog_iterator<
      monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE>> iterator;
  typedef monolog_iterator<
      monolog_linear<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE>> const_iterator;

  /**
   * Default constructor.
   */
  monolog_linear()
      : monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE>(),
        tail_(0UL) {
  }

  /**
   * Constructor to intialize the monolog with specified name, data path and
   * storage mode.
   *
   * @param name The name of the monolog.
   * @param data_path The data path for the monolog.
   * @param storage The storage mode for the monolog.
   */
  monolog_linear(const std::string& name, const std::string& data_path,
                 const storage::storage_mode& storage = storage::IN_MEMORY)
      : monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE>(name,
                                                                    data_path,
                                                                    storage),
        tail_(0UL) {
  }

  /**
   * Add new element to end of monolog, adding more space if necessary.
   *
   * @param val The value to add at the end of monolog.
   *
   * @return The offset that the value was added at.
   */
  size_t push_back(const T& val) {
    size_t idx = atomic::faa(&tail_, 1UL);
    this->set(idx, val);
    return idx;
  }

  /**
   * Add a range of values to the end of the monolog, allocating more space
   * if necessary.
   *
   * @param start The start of the range.
   * @param end The end of the range
   *
   * @return The start offset in the monolog where the range was written.
   */
  size_t push_back_range(const T& start, const T& end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);
    return idx;
  }

  /**
   * Append elements to end of monolog.
   *
   * @param data The element array.
   * @param len The number of elements to append.
   *
   * @return The start offset in the monolog where the data was written.
   */
  size_t append(const T* data, size_t len) {
    size_t offset = atomic::faa(&tail_, len);
    this->write(offset, data, len);
    return offset;
  }

  /**
   * Reserve specified number of elements at the end of the monolog.
   *
   * @param len The number of elements to reserve
   *
   * @return Start offset for the reserved space within the monolog.
   */
  size_t reserve(size_t len) {
    return atomic::faa(&tail_, len);
  }

  /**
   * Get the element at specified index.
   *
   * @param idx The index into the monolog.
   *
   * @return The value at specified index.
   */
  const T at(size_t idx) const {
    return this->get(idx);
  }

  /**
   * Get the size of the monolog.
   *
   * @return The size of the monolog.
   */
  size_t size() const {
    return atomic::load(&tail_);
  }

  /**
   * Get the begin iterator of the monolog.
   *
   * @return The begin iterator.
   */
  iterator begin() const {
    return iterator(this, 0);
  }

  /**
   * Get the end iterator of the monolog.
   *
   * @return The end iterator.
   */
  iterator end() const {
    return iterator(this, size());
  }

 private:
  atomic::type<size_t> tail_;
};

}
}

#endif /* CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_ */

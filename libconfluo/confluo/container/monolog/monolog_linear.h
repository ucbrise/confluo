#ifndef CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_
#define CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_

#include <array>
#include <vector>

#include "atomic.h"
#include "monolog_linear_block.h"
#include "storage/ptr.h"
#include "storage/storage.h"

// TODO: Add documentation

namespace confluo {
namespace monolog {

template<typename T, size_t MAX_BLOCKS = 4096, size_t BLOCK_SIZE = 268435456,
    size_t BUFFER_SIZE = 1048576>
class monolog_linear_base {
 public:
  monolog_linear_base() = default;

  /**
   * monolog_linear_base
   *
   * @param name The name
   * @param data_path The data_path
   * @param storage The storage
   */
  monolog_linear_base(const std::string& name, const std::string& data_path,
                      const storage::storage_mode& storage) {
    init(name, data_path, storage);
  }

  /**
   * init
   *
   * @param name The name
   * @param data_path The data_path
   * @param storage The storage
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
   * name
   *
   * @return std::string
   */
  std::string name() const {
    return name_;
  }

  /**
   * data_path
   *
   * @return std::string
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

  // Get len bytes of data at offset.
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
   * Gets a pointer to the data at idx
   * @param idx monolog index
   * @param data_ptr read-only pointer to store in
   */
  void ptr(size_t idx, storage::read_only_ptr<T>& data_ptr) {
    blocks_[idx / BLOCK_SIZE].ptr(idx % BLOCK_SIZE, data_ptr);
  }

  /**
   * Gets a pointer to the data at idx
   * @param idx monolog index
   * @param data_ptr read-only pointer to store in
   */
  void cptr(size_t idx, storage::read_only_ptr<T>& data_ptr) const {
    blocks_[idx / BLOCK_SIZE].cptr(idx % BLOCK_SIZE, data_ptr);
  }

  T& operator[](size_t idx) {
    return blocks_[idx / BLOCK_SIZE][idx % BLOCK_SIZE];
  }

  void swap_block_ptr(size_t idx, T* ptr) {
    blocks_[idx / BLOCK_SIZE].swap_ptr(ptr);
  }

  /**
   * @return storage size of the monolog
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
   * monolog_linear
   */
  monolog_linear()
      : monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE>(),
        tail_(0UL) {
  }

  /**
   * monolog_linear
   *
   * @param name The name
   * @param data_path The data_path
   * @param storage The storage
   */
  monolog_linear(const std::string& name, const std::string& data_path,
                 const storage::storage_mode& storage = storage::IN_MEMORY)
      : monolog_linear_base<T, MAX_BLOCKS, BLOCK_SIZE, BUFFER_SIZE>(name,
                                                                    data_path,
                                                                    storage),
        tail_(0UL) {
  }

  /**
   * push_back
   *
   * @param val The val
   *
   * @return size_t
   */
  size_t push_back(const T& val) {
    size_t idx = atomic::faa(&tail_, 1UL);
    this->set(idx, val);
    return idx;
  }

  /**
   * push_back_range
   *
   * @param start The start
   * @param end The end
   *
   * @return size_t
   */
  size_t push_back_range(const T& start, const T& end) {
    size_t cnt = (end - start + 1);
    size_t idx = atomic::faa(&tail_, cnt);
    for (size_t i = 0; i < cnt; i++)
      this->set(idx + i, start + i);
    return idx;
  }

  /**
   * append
   *
   * @param data The data
   * @param len The len
   *
   * @return size_t
   */
  size_t append(const T* data, size_t len) {
    size_t offset = atomic::faa(&tail_, len);
    this->write(offset, data, len);
    return offset;
  }

  /**
   * reserve
   *
   * @param len The len
   *
   * @return size_t
   */
  size_t reserve(size_t len) {
    return atomic::faa(&tail_, len);
  }

  /**
   * at
   *
   * @param idx The idx
   *
   * @return T
   */
  const T at(size_t idx) const {
    return this->get(idx);
  }

  /**
   * size
   *
   * @return size_t
   */
  size_t size() const {
    return atomic::load(&tail_);
  }

  /**
   * begin
   *
   * @return iterator
   */
  iterator begin() const {
    return iterator(this, 0);
  }

  /**
   * end
   *
   * @return iterator
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

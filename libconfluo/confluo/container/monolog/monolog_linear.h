#ifndef CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_
#define CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_

#include <array>
#include <vector>

#include "monolog_iterator.h"
#include "storage/swappable_encoded_ptr.h"
#include "atomic.h"
#include "monolog_linear_bucket.h"
#include "storage/storage.h"

namespace confluo {
namespace monolog {

/**
 * Monolog linear base class. The monolog grow linearly, bucket-by-bucket, as
 * space runs out in previous blocks.
 */
template<typename T, size_t MAX_BUCKETS = 4096, size_t BUCKET_SIZE = 268435456, size_t BUFFER_SIZE = 1048576>
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
  monolog_linear_base(const std::string &name, const std::string &data_path,
                      const storage::storage_mode &storage) {
    init(name, data_path, storage);
  }

  /**
   * Copy constructor.
   * Note: only initializes member fields. Any data
   * copying should be done by the super class.
   * @param other other monolog_linear
   */
  monolog_linear_base(const monolog_linear_base &other) {
    init(other.name_, other.data_path_, other.buckets_[0].mode());
  }

  /**
   * Copy assignment.
   * Note: only initializes member fields. Any data
   * copying should be done by the super class.
   * @param other
   * @return this
   */
  monolog_linear_base &operator=(const monolog_linear_base &other) {
    if (&other == this)
      return *this;
    init(other.name_, other.data_path_, other.buckets_[0].mode());
    return *this;
  }

  /**
   * Initialize the monolog with specified name, data path and storage mode.
   *
   * @param name The name of the monolog.
   * @param data_path The data path where the monolog is stored.
   * @param storage The storage mode for the monolog.
   */
  void init(const std::string &name, const std::string &data_path,
            const storage::storage_mode &storage) {
    name_ = name;
    data_path_ = data_path;
    for (size_t i = 0; i < MAX_BUCKETS; i++) {
      buckets_[i].init(bucket_data_path(i), BUCKET_SIZE, storage);
    }
  }

  /**
   * Pre-allocates the first bucket.
   */
  void pre_alloc() {
    buckets_[0].ensure_alloc();
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
   * Get the bucket size.
   * @return The bucket size.
   */
  size_t bucket_size() {
    return BUCKET_SIZE + BUFFER_SIZE;
  }

  /**
   * Get the bucket data path for a bucket index.
   * @param bucket_idx The bucket index.
   * @return The bucket data path.
   */
  std::string bucket_data_path(size_t bucket_idx) const {
    return data_path() + "/" + name() + "_" + std::to_string(bucket_idx) + ".dat";
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
   * Ensures buckets are allocated to cover the range of indexes given.
   * @param start_idx start index
   * @param end_idx end index
   */
  void ensure_alloc(size_t start_idx, size_t end_idx) {
    size_t bucket_idx1 = start_idx / BUCKET_SIZE;
    size_t bucket_idx2 = end_idx / BUCKET_SIZE;
    for (size_t i = bucket_idx1; i <= bucket_idx2; i++) {
      buckets_[i].ensure_alloc();
    }
  }

  /**
   * Sets the data at index idx to val. Allocates memory if necessary.
   * @param idx index to set at
   * @param val value to set
   */
  void set(size_t idx, const T &val) {
    buckets_[idx / BUCKET_SIZE].set(idx % BUCKET_SIZE, val);
  }

  /**
   * Sets the data at index idx to val. Does NOT allocate memory --
   * ensure memory is allocated before calling this function.
   * @param idx index to set at
   * @param val value to set
   */
  void set_unsafe(size_t idx, const T val) {
    buckets_[idx / BUCKET_SIZE].set_unsafe(idx % BUCKET_SIZE, val);
  }

  /**
   * Write len bytes of data at idx. Allocates memory if necessary.
   * @param idx monolog index
   * @param data data to write
   * @param len length of data
   */
  void write(size_t idx, const T *data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = idx / BUCKET_SIZE;
      size_t bucket_off = idx % BUCKET_SIZE;
      size_t bucket_len = std::min(BUCKET_SIZE - bucket_off, remaining);
      buckets_[bucket_idx].write(bucket_off, data + len - remaining, bucket_len);
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
  void write_unsafe(size_t idx, const T *data, size_t len) {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = idx / BUCKET_SIZE;
      size_t bucket_off = idx % BUCKET_SIZE;
      size_t bucket_len = std::min(BUCKET_SIZE - bucket_off, remaining);
      buckets_[bucket_idx].write_unsafe(bucket_off, data + len - remaining, bucket_len);
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
    buckets_[idx / BUCKET_SIZE].flush(idx % BUCKET_SIZE, len);
  }

  /**
   * Gets data at idx
   * @param idx monolog index
   * @return data
   */
  const T get(size_t idx) const {
    return buckets_[idx / BUCKET_SIZE].at(idx % BUCKET_SIZE);
  }

  /**
   * Read a specified number of elements from monolog at a specified offset.
   * @param offset The offset to read from.
   * @param data The buffer that the data will be read into.
   * @param len The number of bytes to read.
   */
  void read(size_t offset, T *data, size_t len) const {
    size_t remaining = len;
    while (remaining) {
      size_t bucket_idx = offset / BUCKET_SIZE;
      size_t bucket_off = offset % BUCKET_SIZE;
      size_t bucket_len = std::min(BUCKET_SIZE - bucket_off, remaining);
      buckets_[bucket_idx].read(bucket_off, data + len - remaining, bucket_len);
      offset += bucket_len;
      remaining -= bucket_len;
    }
  }

  /**
   * Gets a pointer to the data at specified index
   * @param idx monolog index
   * @param data_ptr read-only pointer to store in
   */
  void ptr(size_t idx, storage::read_only_encoded_ptr<T> &data_ptr) {
    buckets_[idx / BUCKET_SIZE].ptr(idx % BUCKET_SIZE, data_ptr);
  }

  /**
   * Gets a pointer to the data at specified index
   * @param idx monolog index
   * @param data_ptr read-only pointer to store in
   */
  void cptr(size_t idx, storage::read_only_encoded_ptr<T> &data_ptr) const {
    buckets_[idx / BUCKET_SIZE].cptr(idx % BUCKET_SIZE, data_ptr);
  }

  /**
   * operator[] (reference)
   * @param idx Index of reference to get.
   * @return Reference to requested data.
   */
  T &operator[](size_t idx) {
    return buckets_[idx / BUCKET_SIZE][idx % BUCKET_SIZE];
  }

  /**
   * Get the storage size of the monolog.
   *
   * @return storage size of the monolog
   */
  size_t storage_size() const;

  /**
   * Note: it's dangerous to modify this data structure.
   * @return pointer to underlying array
   */
  std::array<monolog_linear_bucket<T, BUFFER_SIZE>, MAX_BUCKETS> &data() {
    return buckets_;
  }

 protected:
  /** The name of the monolog */
  std::string name_;
  /** The path for data of the monolog */
  std::string data_path_;
  /** The array of monolog buckets */
  std::array<monolog_linear_bucket<T, BUFFER_SIZE>, MAX_BUCKETS> buckets_;

};

/**
 * Monolog linear class. The monolog grows linearly, bucket-by-bucket,
 * as space runs out in previous blocks.
 */
template<typename T, size_t MAX_BUCKETS = 4096, size_t BUCKET_SIZE = 268435456, size_t BUFFER_SIZE = 1048576>
class monolog_linear : public monolog_linear_base<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE> {
 public:
  // Type definitions
  /** The size type */
  typedef size_t size_type;
  /** The position type */
  typedef size_t pos_type;
  /** The value type */
  typedef T value_type;
  /** The difference type */
  typedef T difference_type;
  /** The pointer type */
  typedef T *pointer;
  /** The reference type */
  typedef T reference;
  /** This type */
  typedef monolog_linear<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE> this_type;
  /** The iterator type */
  typedef monolog_iterator<this_type> iterator;
  /** The constant iterator type */
  typedef monolog_iterator<this_type> const_iterator;
  /** The bucket iterator type */
  typedef monolog_bucket_iterator<this_type, BUCKET_SIZE> bucket_iterator;
  /** The constant bucket iterator type */
  typedef monolog_bucket_iterator<this_type, BUCKET_SIZE> const_bucket_iterator;

  /**
   * Default constructor.
   */
  monolog_linear()
      : monolog_linear_base<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE>(),
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
  monolog_linear(const std::string &name, const std::string &data_path,
                 const storage::storage_mode &storage = storage::IN_MEMORY)
      : monolog_linear_base<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE>(name, data_path, storage),
        tail_(0UL) {
  }

  /**
   * Copy assignment. Copies all data in the other monolog.
   * @param other other monolog_linear
   */
  monolog_linear(const monolog_linear &other)
      : monolog_linear_base<T, MAX_BUCKETS, BUCKET_SIZE, BUFFER_SIZE>(other) {
    atomic::init(&tail_, 0UL);
    for (size_t i = 0; i < other.size(); i++)
      this->push_back(other.get(i));
  }

  /**
   * Copy assignment. Copies all data in the other monolog.
   * @param other other monolog_linear
   * @return this copied monolog_linear
   */
  monolog_linear &operator=(const monolog_linear &other) {
    if (&other == this)
      return *this;
    monolog_linear::operator=(other);
    atomic::init(&tail_, 0UL);
    for (size_t i = 0; i < other.size(); i++)
      this->push_back(other.get(i));
    return *this;
  }

  /**
   * Add a new element to end of monolog, adding more space if necessary.
   *
   * @param val The value to add at the end of monolog.
   *
   * @return The offset that the value was added at.
   *
   */
  size_t push_back(const T &val) {
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
  size_t push_back_range(const T &start, const T &end) {
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
  size_t append(const T *data, size_t len) {
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

  /**
   * Get an iterator over the first bucket
   * @return Iterator over the first bucket
   */
  bucket_iterator begin_bucket() const {
    return bucket_iterator(this, 0);
  }

  /**
   * Get an iterator over the last bucket
   * @return Iterator over the last bucket
   */
  bucket_iterator end_bucket() const {
    return bucket_iterator(this, size() + BUCKET_SIZE - (size() % BUCKET_SIZE));
  }

 private:
  atomic::type<size_t> tail_;
};

}
}

#endif /* CONFLUO_CONTAINER_MONOLOG_MONOLOG_LINEAR_H_ */

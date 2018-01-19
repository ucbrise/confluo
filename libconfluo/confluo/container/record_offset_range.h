#ifndef CONFLUO_CONTAINER_RECORD_OFFSET_RANGE_H_
#define CONFLUO_CONTAINER_RECORD_OFFSET_RANGE_H_

#include <cstdint>
#include <iterator>

namespace confluo {

/**
* Range for record offsets
*/
class record_offset_range {
 public:
  typedef uint64_t value_type;
  /**
   * Iterator
   */
  class iterator : public std::iterator<std::input_iterator_tag, uint64_t,
      uint64_t, const uint64_t*, uint64_t> {
   public:
    typedef uint64_t value_type;

    /**
     * Iterator for this range
     *
     * @param offset The offset for this range
     * @param version The version of the log
     * @param record_size The size of each record
     */
    explicit iterator(uint64_t offset, uint64_t version, uint64_t record_size)
        : offset_(offset),
          version_(version),
          record_size_(record_size) {
    }

    /**
     * Increments iterator to the next record
     *
     * @return This iterator advanced
     */
    iterator& operator++() {
      offset_ += record_size_;
      return *this;
    }

    /**
     * Increments this iterator by a specified amount
     *
     * @param int The amount to advance the iterator by
     *
     * @return A pointer to this iterator advanced
     */
    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }

    /**
     * Performs an equality comparison between this iterator and the other
     * iterator
     *
     * @param other The other iterator to perform the equality comparison
     * against
     *
     * @return True if the iterators are equal, false otherwise
     */
    bool operator==(iterator other) const {
      return offset_ == other.offset_ && version_ == other.version_
          && record_size_ == other.record_size_;
    }

    /**
     * Performs a not equal comparison between this iterator and the
     * iterator passed in
     *
     * @param other The other iterator used for comparison
     *
     * @return True if this iterator is less than the other iterator, false
     * otherwise
     */
    bool operator!=(iterator other) const {
      return !(*this == other);
    }

    /**
     * Dereference this offset range
     *
     * @return The offset
     */
    reference operator*() const {
      return offset_;
    }

   private:
    uint64_t offset_;
    uint64_t version_;
    uint64_t record_size_;
  };

  /**
   * Constructs a offset range from the specified version and record
   * size
   *
   * @param version The version of the log
   * @param record_size The size of each record
   */
  record_offset_range(uint64_t version, uint64_t record_size)
      : version_(version),
        record_size_(record_size) {
  }

  /**
   * Gets the beginning of the iterator
   *
   * @return The beginning of the iterator
   */
  const iterator begin() const {
    return iterator(0, version_, record_size_);
  }

  /**
   * Gets the end of the iterator
   *
   * @return The end of the iterator
   */
  const iterator end() const {
    return iterator(version_, version_, record_size_);
  }

 private:
  uint64_t version_;
  uint64_t record_size_;
};

}

#endif /* CONFLUO_CONTAINER_RECORD_OFFSET_RANGE_H_ */

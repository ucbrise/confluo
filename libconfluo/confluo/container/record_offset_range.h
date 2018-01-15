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
     * iterator
     *
     * @param offset The offset
     * @param version The version
     * @param record_size The record_size
     */
    explicit iterator(uint64_t offset, uint64_t version, uint64_t record_size)
        : offset_(offset),
          version_(version),
          record_size_(record_size) {
    }

    /**
     * operator++
     *
     * @return iterator&
     */
    iterator& operator++() {
      offset_ += record_size_;
      return *this;
    }

    /**
     * operator++
     *
     * @param int The int
     *
     * @return iterator
     */
    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }

    /**
     * operator==
     *
     * @param other The other
     *
     * @return bool
     */
    bool operator==(iterator other) const {
      return offset_ == other.offset_ && version_ == other.version_
          && record_size_ == other.record_size_;
    }

    /**
     * operator!=
     *
     * @param other The other
     *
     * @return bool
     */
    bool operator!=(iterator other) const {
      return !(*this == other);
    }

    /**
     * operator
     *
     * @return reference
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
   * record_offset_range
   *
   * @param version The version
   * @param record_size The record_size
   */
  record_offset_range(uint64_t version, uint64_t record_size)
      : version_(version),
        record_size_(record_size) {
  }

  /**
   * begin
   *
   * @return iterator
   */
  const iterator begin() const {
    return iterator(0, version_, record_size_);
  }

  /**
   * end
   *
   * @return iterator
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

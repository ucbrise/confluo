#ifndef CONFLUO_CONTAINER_RECORD_OFFSET_RANGE_H_
#define CONFLUO_CONTAINER_RECORD_OFFSET_RANGE_H_

#include <cstdint>
#include <iterator>

namespace confluo {

class record_offset_range {
 public:
  typedef uint64_t value_type;
  class iterator : public std::iterator<std::input_iterator_tag, uint64_t,
      uint64_t, const uint64_t*, uint64_t> {
   public:
    typedef uint64_t value_type;

    explicit iterator(uint64_t offset, uint64_t version, uint64_t record_size)
        : offset_(offset),
          version_(version),
          record_size_(record_size) {
    }

    iterator& operator++() {
      offset_ += record_size_;
      return *this;
    }

    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }

    bool operator==(iterator other) const {
      return offset_ == other.offset_ && version_ == other.version_
          && record_size_ == other.record_size_;
    }

    bool operator!=(iterator other) const {
      return !(*this == other);
    }

    reference operator*() const {
      return offset_;
    }

   private:
    uint64_t offset_;
    uint64_t version_;
    uint64_t record_size_;
  };

  record_offset_range(uint64_t version, uint64_t record_size)
      : version_(version),
        record_size_(record_size) {
  }

  const iterator begin() const {
    return iterator(0, version_, record_size_);
  }

  const iterator end() const {
    return iterator(version_, version_, record_size_);
  }

 private:
  uint64_t version_;
  uint64_t record_size_;
};

}

#endif /* CONFLUO_CONTAINER_RECORD_OFFSET_RANGE_H_ */

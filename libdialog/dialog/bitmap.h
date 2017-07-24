#ifndef BITMAP_BITMAP_H_
#define BITMAP_BITMAP_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "bit_utils.h"

namespace dialog {

class bitmap {
 public:
  // Type definitions
  typedef size_t pos_type;
  typedef size_t size_type;
  typedef uint64_t data_type;
  typedef uint8_t width_type;

  // Constructors and Destructors
  bitmap() {
    data_ = NULL;
    size_ = 0;
  }

  bitmap(size_type num_bits) {
    data_ = new data_type[BITS2BLOCKS(num_bits)]();
    size_ = num_bits;
  }

  virtual ~bitmap() {
    if (data_ != NULL) {
      delete[] data_;
      data_ = NULL;
    }
  }

  // Getters
  data_type* data() {
    return data_;
  }

  size_type num_bits() {
    return size_;
  }

  // Bit operations
  void clear_all() {
    memset((void *) data_, 0, BITS2BLOCKS(size_) * sizeof(uint64_t));
  }

  void set_bit(pos_type i) {
    SETBITVAL(data_, i);
  }

  void unset_bit(pos_type i) {
    CLRBITVAL(data_, i);
  }

  bool get_bit(pos_type i) const {
    return GETBITVAL(data_, i);
  }

  // Integer operations
  template<typename T>
  typename std::enable_if<std::is_arithmetic<T>::value>::type set_val_pos(
      pos_type pos, T val, width_type bits) {
    pos_type s_off = pos % 64;
    pos_type s_idx = pos / 64;

    if (s_off + bits <= 64) {
      // Can be accommodated in 1 bitmap block
      data_[s_idx] = (data_[s_idx]
          & (low_bits_set[s_off] | low_bits_unset[s_off + bits]))
          | val << s_off;
    } else {
      // Must use 2 bitmap blocks
      data_[s_idx] = (data_[s_idx] & low_bits_set[s_off]) | val << s_off;
      data_[s_idx + 1] =
          (data_[s_idx + 1] & low_bits_unset[(s_off + bits) % 64])
              | (val >> (64 - s_off));
    }
  }

  template<typename T>
  typename std::enable_if<std::is_arithmetic<T>::value, T>::type get_val_pos(
      pos_type pos, width_type bits) const {
    pos_type s_off = pos % 64;
    pos_type s_idx = pos / 64;

    if (s_off + bits <= 64) {
      // Can be read from a single block
      return static_cast<T>((data_[s_idx] >> s_off) & low_bits_set[bits]);
    } else {
      // Must be read from two blocks
      return static_cast<T>(((data_[s_idx] >> s_off)
          | (data_[s_idx + 1] << (64 - s_off))) & low_bits_set[bits]);
    }
  }

  // Serialization/De-serialization
  virtual size_type serialize(std::ostream& out) {
    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&size_), sizeof(size_type));
    out_size += sizeof(size_type);

    out.write(reinterpret_cast<const char *>(data_),
              sizeof(data_type) * BITS2BLOCKS(size_));
    out_size += (BITS2BLOCKS(size_) * sizeof(uint64_t));

    return out_size;
  }

  virtual size_type deserialize(std::istream& in) {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&size_), sizeof(size_type));
    in_size += sizeof(size_type);

    data_ = new data_type[BITS2BLOCKS(size_)];
    in.read(reinterpret_cast<char *>(data_),
    BITS2BLOCKS(size_) * sizeof(data_type));
    in_size += (BITS2BLOCKS(size_) * sizeof(data_type));

    return in_size;
  }

 protected:
  // Data members
  data_type *data_;
  size_type size_;
};

}

#endif

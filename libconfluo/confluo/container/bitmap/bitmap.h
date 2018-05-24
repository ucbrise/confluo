#ifndef CONFLUO_CONTAINER_BITMAP_BITMAP_H_
#define CONFLUO_CONTAINER_BITMAP_BITMAP_H_

#include <cstdint>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "bit_utils.h"

namespace confluo {

using namespace ::utils;

/**
 * The bitmap class. Stores a bit-array of a fixed size, and supports access and
 * modifications via efficient bitwise arithmetic.
 */
class bitmap {
 public:
  // Type definitions
  /** Position type */
  typedef size_t pos_type;
  /** The size type */
  typedef size_t size_type;
  /** The data type */
  typedef uint64_t data_type;
  /** The width type */
  typedef uint8_t width_type;

  // Constructors and Destructors
  /**
   * Default constructor initializes the data and size of the bitmap to
   * default values
   */
  bitmap() {
    data_ = NULL;
    size_ = 0;
  }

  /**
   * Constructor that initializes the bitmap
   * @param num_bits The size of the bitmap
   */
  bitmap(size_type num_bits) {
    size_t alloc_size = sizeof(data_type) * BITS2BLOCKS(num_bits);
    data_ = static_cast<data_type*>(ALLOCATOR.alloc(alloc_size));
    memset(data_, 0, alloc_size);
    size_ = num_bits;
  }

  /**
   * Default destructor that deletes the bitmap data
   */
  virtual ~bitmap() {
    if (data_ != NULL) {
      ALLOCATOR.dealloc(data_);
      data_ = NULL;
    }
  }

  // Getters
  /**
   * Gets the data
   * @return The data
   */
  data_type *data() {
    return data_;
  }

  /**
   * Gets the number of bits
   * @return The size of the bitmap
   */
  size_type num_bits() {
    return size_;
  }

  // Bit operations
  /**
   * Clears all of the data in the bitmap, by setting all values to 0
   */
  void clear_all() {
    memset((void *) data_, 0, BITS2BLOCKS(size_) * sizeof(uint64_t));
  }

  /**
   * Sets bit at specified index
   * @param i The index
   */
  void set_bit(pos_type i) {
    SETBITVAL(data_, i);
  }

  /**
   * Clears the bit at the specified index
   * @param i The index
   */
  void unset_bit(pos_type i) {
    CLRBITVAL(data_, i);
  }

  /**
   * Gets the bit at the specified index
   * @param i The index
   * @return The bit value at the index
   */
  bool get_bit(pos_type i) const {
    return static_cast<bool>(GETBITVAL(data_, i));
  }

  virtual void set_size(size_type size) {
    size_ = size;
  }

  // Integer operations
  /**
   * Sets the value at a specific position
   * @param pos The position
   * @param val The value
   * @param bits The number of bits
   * @return The value
   */
  template<typename T>
  typename std::enable_if<std::is_arithmetic<T>::value>::type set_val_pos(pos_type pos, T val, width_type bits) {
    pos_type s_off = pos % 64;
    pos_type s_idx = pos / 64;

    if (s_off + bits <= 64) {
      // Can be accommodated in 1 bitmap block
      data_[s_idx] = (data_[s_idx] & (low_bits_set[s_off] | low_bits_unset[s_off + bits])) | val << s_off;
    } else {
      // Must use 2 bitmap blocks
      data_[s_idx] = (data_[s_idx] & low_bits_set[s_off]) | val << s_off;
      data_[s_idx + 1] = (data_[s_idx + 1] & low_bits_unset[(s_off + bits) % 64]) | (val >> (64 - s_off));
    }
  }

  /**
   * Gets the value at the position
   * @param pos The position
   * @param bits The number of bits
   * @return The data at the position
   */
  template<typename T>
  typename std::enable_if<std::is_arithmetic<T>::value, T>::type get_val_pos(pos_type pos, width_type bits) const {
    pos_type s_off = pos % 64;
    pos_type s_idx = pos / 64;

    if (s_off + bits <= 64) {
      // Can be read from a single block
      return static_cast<T>((data_[s_idx] >> s_off) & low_bits_set[bits]);
    } else {
      // Must be read from two blocks
      return static_cast<T>(((data_[s_idx] >> s_off) | (data_[s_idx + 1] << (64 - s_off))) & low_bits_set[bits]);
    }
  }

  // Serialization/De-serialization
  /**
   * Serializes bitmap to an output stream
   * @param out The output stream where the bitmap is serialized
   * @return The size of the data in the stream
   */
  virtual size_type serialize(std::ostream &out) {
    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&size_), sizeof(size_type));
    out_size += sizeof(size_type);

    out.write(reinterpret_cast<const char *>(data_), sizeof(data_type) * BITS2BLOCKS(size_));
    out_size += (BITS2BLOCKS(size_) * sizeof(uint64_t));

    return out_size;
  }

  /**
   * Deserializes bitmap from an input stream
   * @param in The input stream where the bitmap is read from
   * @return The size of the data from the stream
   */
  virtual size_type deserialize(std::istream &in) {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&size_), sizeof(size_type));
    in_size += sizeof(size_type);
    size_t alloc_size = sizeof(data_type) * BITS2BLOCKS(size_);
    data_ = static_cast<data_type *>(ALLOCATOR.alloc(alloc_size));
    memset(data_, 0, alloc_size);
    in.read(reinterpret_cast<char *>(data_), BITS2BLOCKS(size_) * sizeof(data_type));
    in_size += (BITS2BLOCKS(size_) * sizeof(data_type));

    return in_size;
  }

 protected:
  // Data members
  /** The bitmap data */
  data_type *data_;
  /** The bitmap size */
  size_type size_;
};

}

#endif /* CONFLUO_CONTAINER_BITMAP_BITMAP_H_ */

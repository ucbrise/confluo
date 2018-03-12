#ifndef CONFLUO_CONTAINER_BITMAP_DELTA_ENCODED_ARRAY_H_
#define CONFLUO_CONTAINER_BITMAP_DELTA_ENCODED_ARRAY_H_

#include <vector>

#include "bitmap.h"
#include "bitmap_array.h"
#include "bit_utils.h"

using namespace utils;

namespace confluo {

/**
 * Delta-encoded array class. Stores sorted integer data encoded using
 * delta-encoding.
 *
 * @tparam T The data type (can only be integers)
 * @tparam sampling_rate The rate at which data is sampled.
 */
template<typename T, uint32_t sampling_rate = 128>
class delta_encoded_array {
 public:
  /** The size type */
  typedef size_t size_type;
  /** The position type */
  typedef size_t pos_type;
  /** The width type */
  typedef uint8_t width_type;

  /**
   * Default constructor
   */
  delta_encoded_array() {
    samples_ = NULL;
    delta_offsets_ = NULL;
    deltas_ = NULL;
  }

  /**
   * Default destructor that deletes all of the samples, offsets, and deltas
   */
  virtual ~delta_encoded_array() {
    if (samples_) {
      delete samples_;
      samples_ = nullptr;
    }
    if (delta_offsets_) {
      delete delta_offsets_;
      delta_offsets_ = nullptr;
    }
    if (deltas_) {
      delete deltas_;
      deltas_ = nullptr;
    }
  }

  // Serialization and De-serialization
  /**
   * Serializes array to output stream
   * @param out The output stream
   * @return The size of the data in the output stream
   */
  virtual size_type serialize(std::ostream& out) {
    size_type out_size = 0;

    out_size += samples_->serialize(out);
    out_size += delta_offsets_->serialize(out);
    out_size += deltas_->serialize(out);

    return out_size;
  }

  /**
   * Deserializes input stream to an array
   * @param in The input stream
   * @return The size of the array from the input stream
   */
  virtual size_type deserialize(std::istream& in) {
    size_type in_size = 0;

    samples_ = new unsized_bitmap_array<T>();
    in_size += samples_->deserialize(in);
    delta_offsets_ = new unsized_bitmap_array<pos_type>();
    in_size += delta_offsets_->deserialize(in);
    deltas_ = new bitmap();
    in_size += deltas_->deserialize(in);

    return in_size;
  }

 protected:
  /**
   * Get the encoding size for an delta value
   * @param delta The delta value
   * @return The width type of the delta
   */
  virtual width_type encoding_size(T delta) = 0;

  /**
   * Encode the delta values
   * @param deltas The delta values
   * @param num_deltas The number of deltas
   */
  virtual void encode_deltas(T* deltas, size_type num_deltas) = 0;

  /**
   * Encode the delta encoded array
   * @param elements The elements of the array
   * @param num_elements The number of elements
   */
  void encode(T* elements, size_type num_elements) {
    if (num_elements == 0) {
      return;
    }

    T max_sample = 0;
    std::vector<T> samples, deltas;
    std::vector<pos_type> delta_offsets;
    T last_val = 0;
    uint64_t tot_delta_count = 0, delta_count = 0;
    uint64_t delta_enc_size = 0;
    size_type cum_delta_size = 0;
    pos_type max_offset = 0;
    width_type sample_bits, delta_offset_bits;

    for (size_t i = 0; i < num_elements; i++) {
      if (i % sampling_rate == 0) {
        samples.push_back(elements[i]);
        if (elements[i] > max_sample) {
          max_sample = elements[i];
        }
        if (cum_delta_size > max_offset)
          max_offset = cum_delta_size;
        delta_offsets.push_back(cum_delta_size);
        if (i != 0) {
          assert(delta_count == sampling_rate - 1);
          tot_delta_count += delta_count;
          delta_count = 0;
        }
      } else {
        assert(elements[i] > last_val);
        T delta = elements[i] - last_val;
        deltas.push_back(delta);

        delta_enc_size = encoding_size(delta);
        cum_delta_size += delta_enc_size;
        delta_count++;
      }
      last_val = elements[i];
    }
    tot_delta_count += delta_count;

    assert(tot_delta_count == deltas.size());
    assert(samples.size() + deltas.size() == num_elements);
    assert(delta_offsets.size() == samples.size());

    sample_bits = bit_utils::bit_width(max_sample);
    delta_offset_bits = bit_utils::bit_width(max_offset);

    if (samples.size() == 0) {
      samples_ = NULL;
    } else {
      samples_ = new unsized_bitmap_array<T>(&samples[0], samples.size(),
                                             sample_bits);
    }

    if (cum_delta_size == 0) {
      deltas_ = NULL;
    } else {
      deltas_ = new bitmap(cum_delta_size);
      encode_deltas(&deltas[0], deltas.size());
    }

    if (delta_offsets.size() == 0) {
      delta_offsets_ = NULL;
    } else {
      delta_offsets_ = new unsized_bitmap_array<pos_type>(&delta_offsets[0],
                                                          delta_offsets.size(),
                                                          delta_offset_bits);
    }
  }

  /** The samples in the delta encoded array */
  unsized_bitmap_array<T>* samples_;
  /** The offsets in the delta encoded array */
  unsized_bitmap_array<pos_type>* delta_offsets_;
  /** The deltas in the delta encoded array */
  bitmap* deltas_;

 private:
};

/**
 * Class that stores pre-computed prefix sum values for Elias-Gamma encoding.
 */
static struct elias_gamma_prefix_sum {
 public:
  /** The type of block */
  typedef uint16_t block_type;

  /**
   * Default constructor that initializes the prefix sum
   */
  elias_gamma_prefix_sum() {
    for (uint64_t i = 0; i < 65536; i++) {
      uint16_t val = (uint16_t) i;
      uint64_t count = 0, offset = 0, sum = 0;
      while (val && offset <= 16) {
        int N = 0;
        while (!GETBIT(val, offset)) {
          N++;
          offset++;
        }
        offset++;
        if (offset + N <= 16) {
          sum += ((val >> offset) & ((uint16_t) low_bits_set[N])) + (1 << N);
          offset += N;
          count++;
        } else {
          offset -= (N + 1);
          break;
        }
      }
      prefixsum_[i] = (offset << 24) | (count << 16) | sum;
    }
  }

  /**
   * Gets the offset
   * @param i The block
   * @return The offset from the block
   */
  uint8_t offset(const block_type i) const {
    return ((prefixsum_[(i)] >> 24) & 0xFF);
  }

  /**
   * Gets the count of the block
   * @param i The block
   * @return The block count
   */
  uint8_t count(const block_type i) const {
    return ((prefixsum_[i] >> 16) & 0xFF);
  }

  /**
   * Gets the sum of the block
   * @param i The block
   * @return The sum of the block
   */
  uint16_t sum(const block_type i) const {
    return (prefixsum_[i] & 0xFFFF);
  }

 private:
  uint32_t prefixsum_[65536];
} elias_gamma_prefix_table;

/**
 * Delta-encoded array that uses Elias-Gamma encoding.
 *
 * @tparam T The data type
 * @tparam sampling_rate The rate at which data is sampled.
 */
template<typename T, uint32_t sampling_rate = 128>
class elias_gamma_encoded_array : public delta_encoded_array<T, sampling_rate> {
 public:
  /** The size type */
  typedef typename delta_encoded_array<T>::size_type size_type;
  /** The position type */
  typedef typename delta_encoded_array<T>::pos_type pos_type;
  /** The width type */
  typedef typename delta_encoded_array<T>::width_type width_type;

  using delta_encoded_array<T>::encoding_size;
  using delta_encoded_array<T>::encode_deltas;

  /**
   * Default constructor
   */
  elias_gamma_encoded_array()
      : delta_encoded_array<T>() {
  }

  /**
   * Constructor that initializes data and size of the array
   * @param elements The elements of the data
   * @param num_elements The number of elements in the array
   */
  elias_gamma_encoded_array(T* elements, size_type num_elements)
      : delta_encoded_array<T>() {
    this->encode(elements, num_elements);
  }

  /**
   * Default destructor
   */
  virtual ~elias_gamma_encoded_array() {
  }

  /**
   * Gets the element at the specified index
   * @param i The index
   * @return The value at that index
   */
  T get(pos_type i) {
    // Get offsets
    pos_type samples_idx = i / sampling_rate;
    pos_type delta_offsets_idx = i % sampling_rate;
    T val = this->samples_->get(samples_idx);

    if (delta_offsets_idx == 0)
      return val;

    pos_type delta_offset = this->delta_offsets_->get(samples_idx);
    val += prefix_sum(delta_offset, delta_offsets_idx);
    return val;
  }

  /**
   * Accesses the array at the specified index
   * @param i The index
   * @return The value at that index
   */
  T operator[](pos_type i) {
    return get(i);
  }

 private:
  /**
   * Get the encoding size for an delta value
   * @param delta The delta value
   * @return The width type of the delta
   */
  virtual width_type encoding_size(T delta) override {
    return 2 * (bit_utils::bit_width(delta) - 1) + 1;
  }

  /**
   * Encode the delta values
   * @param deltas The delta values
   * @param num_deltas The number of deltas
   */
  virtual void encode_deltas(T *deltas, size_type num_deltas) override {
    uint64_t pos = 0;
    for (size_t i = 0; i < num_deltas; i++) {
      uint64_t delta_bits = bit_utils::bit_width(deltas[i]) - 1;
      pos += delta_bits;
      assert((1ULL << delta_bits) <= deltas[i]);
      this->deltas_->set_bit(pos++);
      this->deltas_->set_val_pos(pos, deltas[i] - (1ULL << delta_bits),
                                 delta_bits);
      pos += delta_bits;
    }
  }

  /**
   * Computes the prefix-sum for data between two specified delta indices.
   * @param delta_offset Offset into the delta array.
   * @param until_idx End-index for prefix-sum computation.
   * @return The computed prefix-sum.
   */
  T prefix_sum(pos_type delta_offset, pos_type until_idx) {
    T delta_sum = 0;
    pos_type delta_idx = 0;
    pos_type current_delta_offset = delta_offset;
    while (delta_idx != until_idx) {
      uint16_t block = this->deltas_->template get_val_pos<uint16_t>(
          current_delta_offset, 16);
      uint16_t cnt = elias_gamma_prefix_table.count(block);
      if (cnt == 0) {
        // If the prefixsum table for the block returns count == 0
        // this must mean the value spans more than 16 bits
        // read this manually
        width_type delta_width = 0;
        while (!this->deltas_->get_bit(current_delta_offset)) {
          delta_width++;
          current_delta_offset++;
        }
        current_delta_offset++;
        delta_sum += this->deltas_->template get_val_pos<T>(
            current_delta_offset, delta_width) + (1ULL << delta_width);
        current_delta_offset += delta_width;
        delta_idx += 1;
      } else if (delta_idx + cnt <= until_idx) {
        // If sum can be computed from the prefixsum table
        delta_sum += elias_gamma_prefix_table.sum(block);
        current_delta_offset += elias_gamma_prefix_table.offset(block);
        delta_idx += cnt;
      } else {
        // Last few values, decode them without looking up table
        while (delta_idx != until_idx) {
          width_type delta_width = 0;
          while (!this->deltas_->get_bit(current_delta_offset)) {
            delta_width++;
            current_delta_offset++;
          }
          current_delta_offset++;
          delta_sum += this->deltas_->template get_val_pos<T>(
              current_delta_offset, delta_width) + (1ULL << delta_width);
          current_delta_offset += delta_width;
          delta_idx += 1;
        }
      }
    }
    return delta_sum;
  }
};

}

#endif /* CONFLUO_CONTAINER_BITMAP_DELTA_ENCODED_ARRAY_H_ */

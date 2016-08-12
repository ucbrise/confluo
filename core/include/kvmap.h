#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include "faclog.h"
#include "utils.h"

namespace slog {

typedef __faclog_base <uint32_t, 32> value_offset_list;

// TODO: A lot of repeated code; cleanup.
class deleted_offsets {
 public:
  static const uint32_t FBS = 16;
  static const uint32_t FBS_HIBIT = 4;
  static const uint32_t NBUCKETS = 32;

  typedef std::atomic<uint32_t> atomic_type;
  typedef std::atomic<atomic_type*> atomic_bucket_ref;

  deleted_offsets() {
    atomic_type* null_ptr = NULL;
    for (auto& x : buckets_)
      x = null_ptr;
    buckets_[0] = new atomic_type[FBS];
  }

  void set(uint32_t idx, const uint32_t val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    if (buckets_[bucket_idx] == NULL)
      try_allocate_bucket(bucket_idx);
    buckets_[bucket_idx][bucket_off] = val;
  }

  uint32_t get(const uint32_t idx) const {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;
    return buckets_[bucket_idx][bucket_off];
  }

  bool update(const uint32_t idx, const uint32_t val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = bit_utils::highest_bit(pos);
    uint32_t bucket_off = pos ^ (1 << hibit);
    uint32_t bucket_idx = hibit - FBS_HIBIT;

    uint32_t current_val;
    bool success;
    do {
      current_val = buckets_[bucket_idx][bucket_off];
      success = std::atomic_compare_exchange_weak(
          &buckets_[bucket_idx][bucket_off], &current_val, val);
    } while ((!current_val || val < current_val) && !success);
    return success;
  }

  const uint32_t serialize(std::ostream& out, uint32_t sz) {
    uint32_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&(sz)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    for (uint32_t i = 0; i < sz; i++) {
      uint32_t val = get(i);
      out.write(reinterpret_cast<const char *>(&val), sizeof(uint32_t));
      out_size += sizeof(uint32_t);
    }

    return out_size;
  }

  uint32_t deserialize(std::istream& in, uint32_t *sz) {
    // Read keys
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(sz), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    for (size_t i = 0; i < *sz; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(uint32_t));
      set(i, val);
      in_size += sizeof(uint32_t);
    }

    return in_size;
  }

 protected:
  void try_allocate_bucket(uint32_t bucket_idx) {
    uint32_t size = (1U << (bucket_idx + FBS_HIBIT));
    atomic_type* new_bucket = new atomic_type[size];
    atomic_type* null_ptr = NULL;

    // Only one thread will be successful in replacing the NULL reference with newly
    // allocated bucket.
    if (std::atomic_compare_exchange_weak(&buckets_[bucket_idx], &null_ptr,
                                          new_bucket)) {
      return;
    }

    // All other threads will deallocate the newly allocated bucket.
    delete[] new_bucket;
  }

  std::array<atomic_bucket_ref, NBUCKETS> buckets_;
};

}

#endif /* SLOG_KVMAP_H_ */

#ifndef SLOG_KVMAP_H_
#define SLOG_KVMAP_H_

#include <cstdint>

#include "splitordered/hashtable.h"
#include "faclog.h"
#include "utils.h"

namespace slog {

#define VALID UINT_MAX

struct vo_entry {
  std::atomic<uint32_t> valid_mark;
  uint32_t offset;

  // default constructor
  vo_entry() {
    this->offset = 0;
    this->valid_mark.store(VALID);
  }

  vo_entry(uint32_t offset) {
    this->offset = offset;
    this->valid_mark.store(VALID);
  }

  vo_entry(const vo_entry& other) {
    this->offset = other.offset;
    this->valid_mark.store(other.valid_mark.load());
  }

  bool invalidate(const uint32_t mark) {
    bool success;
    uint32_t current_mark;
    current_mark = valid_mark.load();

    // If entry was invalidated by an earlier operation, no need to invalidate
    if (mark > current_mark)
      return false;

    do {
      success = valid_mark.compare_exchange_weak(current_mark, mark);
    } while (mark < current_mark && !success);  // Repeat until success, or an
                                                // earlier operation succeeds
                                                // in invalidating

    return success;
  }

  bool is_valid(const uint32_t mark) {
    return valid_mark == VALID || mark < valid_mark;
  }
};

struct kvo_entry {
  int64_t key;
  vo_entry vo;
};

class sysgen_kvmap {
 public:
  sysgen_kvmap() {
  }

  void add_internal(uint32_t key, uint32_t value_offset) {
    vo_entries_[key].offset = value_offset;
  }

  bool invalidate_internal(uint32_t key, uint32_t mark) {
    return vo_entries_[key].invalidate(mark);
  }

  bool is_valid_internal(uint32_t key, uint32_t mark) {
    return vo_entries_.get(key).is_valid(mark);
  }

  bool fwd_lookup_internal(uint32_t key, uint32_t max_key, uint32_t mark,
                           uint32_t* offset_begin, uint32_t* offset_end) {
    vo_entry entry = vo_entries_.get(key);
    *offset_begin = entry.offset;
    *offset_end = (key + 1 < max_key) ? vo_entries_.get(key + 1).offset : mark;
    return entry.is_valid(mark);
  }

  bool bwd_lookup_internal(uint32_t value_offset, uint32_t mark, uint32_t min,
                           uint32_t max, uint32_t* key) {
    uint32_t lo = min, hi = max;
    while (lo < hi) {
      uint32_t mid = lo + (hi - lo) / 2;
      if (vo_entries_.get(mid).offset <= value_offset)
        lo = mid + 1;
      else
        hi = mid;
    }

    // The internal key where the search ended.
    *key = lo - 1;
    return vo_entries_.get(*key).is_valid(mark);
  }

  void add_external(int64_t external_key, uint32_t internal_key, uint32_t mark) {
    assert(((uint32_t) external_key) == internal_key);
  }

  bool ext_to_int(int64_t external_key, uint32_t* internal_key) {
    *internal_key = (uint32_t) external_key;
    return true;
  }

  bool int_to_ext(uint32_t internal_key, int64_t* external_key) {
    *external_key = (int64_t) internal_key;
    return true;
  }

 private:
  __faclog_base <vo_entry, 32> vo_entries_;
};

class udef_kvmap {
 public:
  udef_kvmap() {
  }

  void add_internal(uint32_t key, uint32_t value_offset) {
    kvo_entries_[key].vo.offset = value_offset;
  }

  bool invalidate_internal(uint32_t key, uint32_t mark) {
    return kvo_entries_[key].vo.invalidate(mark);
  }

  bool is_valid_internal(uint32_t key, uint32_t mark) {
    return kvo_entries_.get(key).vo.is_valid(mark);
  }

  bool fwd_lookup_internal(uint32_t key, uint32_t max_key, uint32_t mark,
                           uint32_t* offset_begin, uint32_t* offset_end) {
    kvo_entry entry = kvo_entries_.get(key);
    *offset_begin = entry.vo.offset;
    *offset_end =
        (key + 1 < max_key) ? kvo_entries_.get(key + 1).vo.offset : mark;
    return entry.vo.is_valid(mark);
  }

  bool bwd_lookup_internal(uint32_t value_offset, uint32_t mark, uint32_t min,
                           uint32_t max, uint32_t* key) {
    uint32_t lo = min, hi = max;
    while (lo < hi) {
      uint32_t mid = lo + (hi - lo) / 2;
      if (kvo_entries_.get(mid).vo.offset <= value_offset)
        lo = mid + 1;
      else
        hi = mid;
    }

    // The internal key where the search ended.
    *key = lo - 1;
    return kvo_entries_.get(*key).vo.is_valid(mark);
  }

  void add_external(int64_t external_key, uint32_t internal_key, uint32_t mark) {
    kvo_entries_[internal_key].key = external_key;
    uint32_t old_internal;
    bool updated = fwd_map_.upsert(external_key, internal_key, &old_internal);
    if (updated)
      invalidate_internal(old_internal, mark);
  }

  bool ext_to_int(int64_t external_key, uint32_t* internal_key) {
    return fwd_map_.get(external_key, internal_key);
  }

  bool int_to_ext(uint32_t internal_key, int64_t* external_key) {
    *external_key = (int64_t) kvo_entries_.get(internal_key).key;
    return true;
  }

 private:
  __faclog_base <kvo_entry, 32> kvo_entries_;
  splitordered::hash_table<uint32_t> fwd_map_;
};

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

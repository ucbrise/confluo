#ifndef VALUE_OFFSETS_H_
#define VALUE_OFFSETS_H_

#include "conc_vectors.h"

typedef __LockFreeBase <uint32_t, 32> ValueOffsets;

class DeletedOffsets : public __LockFreeBaseAtomic<uint32_t, 32> {
 public:
  DeletedOffsets() {
  }

  bool update(const uint32_t idx, const uint32_t val) {
    uint32_t pos = idx + FBS;
    uint32_t hibit = HighestBit(pos);
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
};

#endif /* VALUE_OFFSETS_H_ */

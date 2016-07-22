#ifndef LOG_STORE_H_
#define LOG_STORE_H_

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <atomic>
#include <cassert>

#include "flags.h"
#include "hash_ops.h"
#include "locks.h"
#include "value_offsets.h"
#include "offset_list.h"
#include "ngram_idx.h"
#include "memory_map.h"

#include "logging.h"

namespace succinct {

template<uint32_t MAX_KEYS = 134217728, uint32_t LOG_SIZE = 1073741824>
class LogStore {
 public:
  // The internal key component of the tail increment for appends and updates.
  static const uint64_t KEY_INCR = 1ULL << 32;

  // The tail increment for a delete operation; we don't increment the internal
  // key, only the value offset by one byte.
  static const uint64_t DEL_INCR = 1ULL;

  // Constructor to initialize the LogStore.
  LogStore() {
    // Initialize the log store to a constant size.
    // Note we can use a lock-free exponentially or linear growing allocator
    // to make the Log dynamically sized rather than static.
    data_ = new char[LOG_SIZE];

    // Initialize both ongoing and completed appends tail to 0.
    ongoing_appends_tail_ = 0;
    completed_appends_tail_ = 0;
    value_offsets_ = new ValueOffsets;
    deleted_ = new DeletedOffsets;
    ngram_idx_ = new NGramIdx;
  }

  // Adds a new key value pair to the LogStore atomically.
  //
  // Returns 0 for a successful append, -1 otherwise.
  int Append(const int64_t key, const std::string& value) {
    // Add the value to the Log and generate and advance the ongoing
    // appends tail.
    uint64_t current_tail = InternalAppend(value);

    // Obtain the tail increment for the value.
    uint64_t tail_increment = TailIncrement(value.length());

    // This is where the user-defined key to internal key
    // mapping would be created.
    //
    // key_mapping_.add(key, current_tail >> 32);

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    AtomicUpdateCompletedAppendsTail(current_tail, tail_increment);

    // Return 0 for success
    return 0;
  }

  // Internal append operation to add a value and its corresponding index
  // entries to the LogStore, and generate an internal key atomically.
  // Advances the ongoing appends tail, but does not update the completed
  // appends tail. This operation is always successful.
  //
  // Returns the ongoing appends tail that this append saw when it started.
  uint64_t InternalAppend(const std::string& value) {
    // Atomically update the ongoing append tail of the log
    uint64_t tail_increment = TailIncrement(value.length());
    uint64_t current_tail = AtomicUpdateOngoingAppendsTail(tail_increment);

    // This thread now has exclusive access to
    // (1) the current internal key, and
    // (2) the region marked by (current value offset, current value offset + current value length)
    uint32_t internal_key = current_tail >> 32;
    uint32_t value_offset = current_tail & 0xFFFFFFFF, value_length = value
        .length();

    // Throw an exception if internal key greater than the largest valid
    // internal key or end of the value goes beyond maximum Log size.
    if (internal_key >= MAX_KEYS || value_offset + value_length >= LOG_SIZE)
      throw -1;

    // We can add the new value offset to the value offsets array
    // and initialize its delete tail without worrying about locking,
    // since we have uncontested access to the 'internal_key' index in the
    // value offsets array and the deleted entries array.
    value_offsets_->set(internal_key, value_offset);
    deleted_->set(internal_key, 0);

    // Similarly, we can append the value to the log without locking
    // since this thread has exclusive access to the region (value_offset, value_offset + .
    memcpy(data_ + value_offset, value.c_str(), value_length);

    // Safely update secondary index entries, in a lock-free manner.
    uint32_t value_end = value_offset + value_length;
    for (uint32_t i = value_offset; i <= value_end - NGRAM_N; i++)
      ngram_idx_->add_offset(data_ + i, i);

    // Return the current tail
    return current_tail;
  }

  // Fetch a value from the LogStore by its internal key.
  //
  // Returns 0 if the fetch is successful, -1 otherwise.
  const int Get(char* value, const uint32_t internal_key) {
    // Get the current completed appends tail, and get the maximum valid key.
    uint64_t current_tail = completed_appends_tail_;
    uint32_t max_key = current_tail >> 32;

    // If requested internal key is < max_key, the write
    // for the internal key hasn't completed yet. Return -1
    // indicating failure.
    if (internal_key >= max_key)
      return -1;

    // Get the delete tail for the internal key, and the current read tail.
    uint32_t delete_tail = deleted_->get(internal_key);
    uint32_t read_tail = current_tail & 0xFFFFFFFF;

    // If the delete tail is non zero (i.e., the value has been deleted),
    // and the read tail is greater than the delete tail, then the
    // value must have been deleted after the read began; return -1 to indicate
    // get failure.
    if (delete_tail && read_tail >= delete_tail)
      return -1;

    // Get the beginning and end offset for the value if key is valid.
    uint32_t start = value_offsets_->get(internal_key);
    uint32_t end =
        (internal_key + 1 < max_key) ?
            value_offsets_->get(internal_key + 1) : current_tail & 0xFFFFFFFF;

    char* data_ptr = data_ + start;
    uint32_t i = 0;
    for (; i < end - start && data_ptr[i] != 0; i++)
      value[i] = data_ptr[i];
    value[i] = '\0';

    // Return 0 for successful get.
    return 0;
  }

  // Search the LogStore for a query string.
  //
  // Returns the set of valid, matching internal keys.
  const void Search(std::set<int64_t>& results, const std::string& query) {
    // Get the current completed appends tail, and extract the maximum valid
    // key and value offset.
    uint64_t current_tail = completed_appends_tail_;
    uint32_t max_key = current_tail >> 32;
    uint32_t max_off = current_tail & 0xFFFFFFFF;

    // Obtain the offsets into the values corresponding to the prefix and
    // suffix ngram for the substring from the N-gram index.
    char *substr = (char *) query.c_str();
    size_t substr_len = query.length();
    assert(substr_len >= NGRAM_N);

    OffsetList* offsets = NULL;
    uint32_t min_size = LOG_SIZE, ngram_off = 0;
    for (uint32_t i = 0; i < substr_len - NGRAM_N; i++) {
      OffsetList *cur = ngram_idx_->get_offsets(substr + i);
      Debug("Cur size = %u", cur->size());
      if (cur->size() < min_size) {
        min_size = cur->size();
        offsets = cur;
        ngram_off = i;
      }
    }
    assert(offsets != NULL);
    Debug("Min size = %u", min_size);

    // Scan through the list of offsets, adding only valid offsets into the
    // set of results.
    for (uint32_t i = 0; i < min_size; i++) {
      // An offset is valid if
      // (1) the remaining query suffix matches the data at that location in
      //     the log,
      // (2) the location does not exceed the maximum valid offset (i.e., the
      //     write at that location was incomplete when the search started)
      // (3) the key is not larger than the maximum valid key (i.e., the write
      //     for that key was incomplete when the search started)
      // (4) the key was not deleted before the search began.
      //
      // TODO: Take care of query.length() <= NGRAM_N case
      uint32_t o = offsets->at(i);
      if (o < max_off && !strncmp(data_ + o - ngram_off, substr, substr_len))
        FindAndInsertKey(results, o, max_key, max_off);
    }
  }

  // Search the LogStore for a column value.
  //
  // Returns the set of valid, matching internal keys.
  const void ColSearch(std::vector<int64_t>& results,
                       const std::string& query) {

    // Get the current completed appends tail, and extract the maximum valid
    // key and value offset.
    uint64_t current_tail = completed_appends_tail_;
    uint32_t max_key = current_tail >> 32;
    uint32_t max_off = current_tail & 0xFFFFFFFF;

    // Obtain the offsets into the values corresponding to the prefix and
    // suffix ngram for the substring from the N-gram index.
    char *substr = (char *) query.c_str();
    size_t substr_len = query.length();
    assert(substr_len >= NGRAM_N);

    OffsetList* offsets = NULL;
    uint32_t min_size = LOG_SIZE, ngram_off = 0;
    for (uint32_t i = 0; i < substr_len - NGRAM_N; i++) {
      OffsetList *cur = ngram_idx_->get_offsets(substr + i);
      if (cur->size() < min_size) {
        min_size = cur->size();
        offsets = cur;
        ngram_off = i;
      }
    }
    assert(offsets != NULL);

    // Scan through the list of offsets, adding only valid offsets into the
    // set of results.
    for (uint32_t i = 0; i < min_size; i++) {
      // An offset is valid if
      // (1) the remaining query suffix matches the data at that location in
      //     the log,
      // (2) the location does not exceed the maximum valid offset (i.e., the
      //     write at that location was incomplete when the search started)
      // (3) the key is not larger than the maximum valid key (i.e., the write
      //     for that key was incomplete when the search started)
      // (4) the key was not deleted before the search began.
      //
      // TODO: Take care of query.length() <= NGRAM_N case
      uint32_t o = offsets->at(i);
      if (o < max_off && !strncmp(data_ + o - ngram_off, substr, substr_len))
        FindAndInsertKey(results, o, max_key, max_off);
    }
  }

  bool InvalidateKey(const uint32_t internal_key, const uint32_t offset) {
    return deleted_->update(internal_key, offset);
  }

  // Atomically deletes the key from the LogStore.
  //
  // Returns true if the delete is successful, false if the key was already
  // deleted or not yet created.
  bool Delete(const uint32_t internal_key) {
    // Atomically increase the ongoing append tail of the log.
    uint64_t current_tail = AtomicUpdateOngoingAppendsTail(DEL_INCR);

    // Obtain the offset into the Log corresponding to the current Log.
    uint32_t value_offset = current_tail & 0xFFFFFFFF;

    // Throw an exception if the delete causes the Log to grow beyond the
    // maximum Log size.
    if (value_offset + 1 >= LOG_SIZE)
      throw -1;

    if (internal_key >= current_tail >> 32)
      return false;

    // Invalidate the given internal key.
    if (InvalidateKey(internal_key, value_offset + 1)) {
      // Atomically update the completed append tail of the log.
      // This is done using CAS, and may have bounded waiting until
      // all appends before the current_tail are completed.
      AtomicUpdateCompletedAppendsTail(current_tail, DEL_INCR);
      return true;
    }

    return false;
  }

  // Atomically removes an existing key, and adds a new value.
  //
  // Returns the internal key associated with the value.
  uint32_t Update(const uint32_t internal_key, const std::string& value) {
    // Add the new value to the Log and generate and advance the ongoing
    // appends tail.
    uint64_t current_tail = InternalAppend(value);

    // Obtain the tail increment for the new value.
    uint64_t tail_increment = TailIncrement(value.length());

    // Invalidate the old internal key. Don't care about the outcome.
    InvalidateKey(internal_key, current_tail & 0xFFFFFFFF + 1);

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    AtomicUpdateCompletedAppendsTail(current_tail, tail_increment);

    return current_tail >> 32;
  }

  // Atomically get the number of currently readable keys.
  const uint32_t GetNumKeys() {
    uint64_t current_tail = completed_appends_tail_;
    return current_tail >> 32;
  }

  // Atomically get the size of the currently readable portion of the LogStore.
  const uint32_t GetSize() {
    uint64_t current_tail = completed_appends_tail_;
    return current_tail & 0xFFFFFFFF;
  }

  // Get the difference between the ongoing appends tail and the completed
  // appends tail. Note that the operation is not atomic, and should only be
  // used for approximate measurements.
  const uint64_t GetGap() {
    return ongoing_appends_tail_ - completed_appends_tail_;
  }

 private:
  // Compute the tail increment for a given value length.
  //
  // Returns the tail increment.
  uint64_t TailIncrement(uint32_t value_length) {
    return KEY_INCR | value_length;
  }

  // Atomically advance the ongoing appends tail by the given amount.
  //
  // Returns the tail value just before the advance occurred.
  uint64_t AtomicUpdateOngoingAppendsTail(uint64_t tail_increment) {
    return std::atomic_fetch_add(&ongoing_appends_tail_, tail_increment);
  }

  // Atomically advance the completed appends tail by the given amount.
  // Waits if there are appends before the expected append tail.
  void AtomicUpdateCompletedAppendsTail(uint64_t expected_append_tail,
                                        uint64_t tail_increment) {
    while (!std::atomic_compare_exchange_weak(
        &completed_appends_tail_, &expected_append_tail,
        expected_append_tail + tail_increment))
      ;
  }

  // Finds the internal key given data offset, the maximum valid key and offset,
  // and inserts the key into the provided set if it hasn't been deleted.
  const void FindAndInsertKey(std::set<int64_t>& keys, const uint32_t offset,
                              const uint32_t max_key, const uint32_t max_off) {

    // Binary search for the offset in the list of value offsets.
    uint32_t lo = 0, hi = max_key;
    while (lo < hi) {
      uint32_t mid = lo + (hi - lo) / 2;
      uint32_t v = value_offsets_->get(mid);
      if (v <= offset)
        lo = mid + 1;
      else
        hi = mid;
    }

    // The internal key where the search ended.
    uint32_t internal_key = lo - 1;

    // Get the delete tail for the internal key.
    uint32_t delete_tail = deleted_->get(internal_key);

    // If the delete tail is non zero (i.e., the key has been deleted),
    // and the current max offset is greater than the delete tail, then the
    // value must have been deleted after the read began; return without
    // inserting internal key into the result set.
    if (delete_tail && max_off >= delete_tail)
      return;

    // Insert the internal key where the search (successfully) ended.
    keys.insert(internal_key);
  }

  // Finds the internal key given data offset, the maximum valid key and offset,
  // and inserts the key into the provided set if it hasn't been deleted.
  const void FindAndInsertKey(std::vector<int64_t>& keys, const uint32_t offset,
                              const uint32_t max_key, const uint32_t max_off) {

    // Binary search for the offset in the list of value offsets.
    uint32_t lo = 0, hi = max_key;
    while (lo < hi) {
      uint32_t mid = lo + (hi - lo) / 2;
      uint32_t v = value_offsets_->get(mid);
      if (v <= offset)
        lo = mid + 1;
      else
        hi = mid;
    }

    // The internal key where the search ended.
    uint32_t internal_key = lo - 1;

    // Get the delete tail for the internal key.
    uint32_t delete_tail = deleted_->get(internal_key);

    // If the delete tail is non zero (i.e., the key has been deleted),
    // and the current max offset is greater than the delete tail, then the
    // value must have been deleted after the read began; return without
    // inserting internal key into the result set.
    if (delete_tail && max_off >= delete_tail)
      return;

    // Insert the internal key where the search (successfully) ended.
    keys.push_back(internal_key);
  }

  char *data_;                                     // Actual log data.
  std::atomic<uint64_t> ongoing_appends_tail_;     // Ongoing appends tail.
  std::atomic<uint64_t> completed_appends_tail_;   // Completed appends tail.
  ValueOffsets *value_offsets_;                    // Lock-free, dynamically
                                                   // growing list of value
                                                   // offsets.
  DeletedOffsets *deleted_;                        // Lock-free, dynamically
                                                   // growing list of atomic
                                                   // delete tails (offsets).
  NGramIdx *ngram_idx_;                            // Lock-free dynamically
                                                   // Growing index mapping
                                                   // N-grams to their locations
                                                   // in the log.
};
}

#endif

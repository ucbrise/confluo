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

#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <atomic>

#include "flags.h"
#include "hash_ops.h"
#include "locks.h"
#include "value_offsets.h"
#include "offset_list.h"
#include "ngram_idx.h"
#include "memory_map.h"

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
  LogStore()
      : ongoing_appends_tail_(0),     // Initialize both ongoing and completed
        completed_appends_tail_(0) {  // appends tail to 0.
    // Initialize the log store to a constant size.
    // Note we can use a lock-free exponentially or linear growing allocator
    // to make the Log dynamically sized rather than static.
    data_ = new char[LOG_SIZE];
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
    uint32_t internal_key = current_tail << 32;
    uint32_t value_offset = current_tail & 0xFFFFFFFF, value_length = value
        .length();

    // Throw an exception if internal key greater than the largest valid
    // internal key or end of the value goes beyond maximum Log size.
    if (internal_key >= MAX_KEYS || value_offset + value_length >= LOG_SIZE)
      throw -1;

    // We can add the new value offset to the value offsets array
    // without worrying about locking, since we have uncontested
    // access to the 'internal_key' index in the value offsets array.
    value_offsets_.set(internal_key, value_offset);

    // Similarly, we can append the value to the log without locking
    // since this thread has exclusive access to the region (value_offset, value_offset + .
    memcpy(data_ + value_offset, value.c_str(), value_length);

    // Safely update secondary index entries, in a lock-free manner.
    uint32_t value_end = value_offset + value_length;
    for (uint32_t i = value_offset; i < value_end - NGRAM_N; i++)
      ngram_idx_.add_offset(data_ + i, i);

    // Return the current tail
    return current_tail;
  }

  // Fetch a value from the LogStore by its internal key.
  //
  // Returns 0 if the fetch is successful, -1 otherwise.
  const int Get(std::string& value, const uint32_t internal_key) {
    // Get the current completed appends tail, and get the maximum valid key.
    uint64_t current_tail = completed_appends_tail_;
    uint32_t max_key = current_tail >> 32;

    // If requested internal key is < max_key, the write
    // for the internal key hasn't completed yet. Return -1
    // indicating failure.
    if (internal_key >= max_key)
      return -1;

    // Get the beginning and end offset for the value if key is valid.
    uint32_t start = value_offsets_.get(internal_key);
    uint32_t end =
        (internal_key + 1 < max_key) ?
            value_offsets_.get(internal_key + 1) : current_tail & 0xFFFFFFFF;

    // Copy the value data into the value.
    value.assign(data_ + start, end - start);

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

    // Extract the substring from the query, and the remaining suffix to
    // compare with the actual data.
    char *substr = (char *) query.c_str();
    char *suffix = substr + NGRAM_N;
    size_t suffix_len = query.length() - NGRAM_N;

    // Obtain the offsets into the values corresponding to the substring ngram
    // from the N-gram index.
    OffsetList* offsets = ngram_idx_.get_offsets(substr);

    // Scan through the list of offsets, adding only valid offsets into the
    // set of results.
    uint32_t size = offsets->size();
    for (uint32_t i = 0; i < size; i++) {
      // An offset is valid if
      // (1) the remaining query suffix matches the data at that location in
      //     the log,
      // (2) the key is not larger than the maximum valid key (i.e., the write
      //     for that key was incomplete when the search started)
      // (3) the key was not deleted before the search began.
      if (!strncmp(data_ + offsets->at(i) + NGRAM_N, suffix, suffix_len)) {
        // TODO: Take care of query.length() <= ngram_n_ case
        int64_t key = GetKey(offsets->at(i), max_key);
        if (key >= 0 && key < max_key)
          results.insert(key);
      }
    }
  }

  void InvalidateKey(const uint32_t internal_key, const uint64_t tail) {

  }

  int Delete(const uint32_t internal_key) {
    // Atomically increase the ongoing append tail of the log.
    uint64_t current_tail = AtomicUpdateOngoingAppendsTail(DEL_INCR);

    // Obtain the offset into the Log corresponding to the current Log.
    uint32_t value_offset = current_tail & 0xFFFFFFFF;

    // Throw an exception if the delete causes the Log to grow beyond the
    // maximum Log size.
    if (value_offset + 1 >= LOG_SIZE)
      throw -1;

    // Invalidate the given internal key.
    InvalidateKey(internal_key, current_tail);

    // Return 0 on a successful delete.
    return 0;
  }

  uint32_t Update(const uint32_t internal_key, const std::string& value) {
    // Add the new value to the Log and generate and advance the ongoing
    // appends tail.
    uint64_t current_tail = InternalAppend(value);

    // Obtain the tail increment for the new value.
    uint64_t tail_increment = TailIncrement(value.length());

    // Invalidate the old internal key.
    InvalidateKey(internal_key, current_tail);

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    AtomicUpdateCompletedAppendsTail(current_tail, tail_increment);

    return 0;
  }

  // Dumps the entire LogStore data to the specified path.
  //
  // Returns the number of bytes written.
  const size_t Dump(const std::string& path) {
    size_t out_size = 0;
    uint64_t tail = completed_appends_tail_;
    uint32_t log_size = tail & 0xFFFFFFFF;
    uint32_t max_key = tail >> 32;

    std::ofstream out(path);

    // Write data
    out.write(reinterpret_cast<const char *>(&(log_size)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    out.write(reinterpret_cast<const char *>(data_), log_size * sizeof(char));
    out_size += (log_size * sizeof(char));

    // Write value offsets
    out_size += value_offsets_.serialize(out, max_key);

    // Write n-gram index
    out_size += ngram_idx_.serialize(out);

    return out_size;
  }

  // Loads the LogStore from the dump at specified path.
  //
  // Returns the number of bytes read.
  size_t Load(const std::string& path) {
    size_t in_size = 0;
    uint32_t log_size, max_key;

    std::ifstream in(path);

    // Read data
    in.read(reinterpret_cast<char *>(&log_size), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    in.read(reinterpret_cast<char *>(data_), log_size * sizeof(char));
    in_size += (log_size * sizeof(char));

    // Read value offsets
    in_size += value_offsets_.deserialize(in, &max_key);

    // Read n-gram index
    in_size += ngram_idx_.deserialize(in);

    // Set the tail values
    ongoing_appends_tail_ = ((uint64_t) max_key) << 32 | ((uint64_t) log_size);
    completed_appends_tail_ = ((uint64_t) max_key) << 32
        | ((uint64_t) log_size);

    return in_size;
  }

  // Atomically get the number of currently readable keys.
  const size_t GetNumKeys() {
    uint64_t current_tail = completed_appends_tail_;
    return current_tail >> 32;
  }

  // Atomically get the size of the currently readable portion of the LogStore.
  const int64_t GetSize() {
    uint64_t current_tail = completed_appends_tail_;
    return current_tail & 0xFFFFFFFF;
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

  // Get the internal key given the value offset and the maximum valid key.
  //
  // Returns -1 if there is no valid key corresponding to the value offset,
  // returns the corresponding internal key otherwise.
  const int64_t GetKey(const uint32_t value_offset, const uint32_t max_key) {
    auto begin = value_offsets_.begin();
    auto end = value_offsets_.end(max_key);
    int64_t internal_key = std::prev(std::upper_bound(begin, end, value_offset))
        - begin;
    return (internal_key >= max_key || internal_key < 0) ? -1 : internal_key;
  }

  char *data_;                                    // Actual log data.
  std::atomic<uint64_t> ongoing_appends_tail_;    // Ongoing appends tail.
  std::atomic<uint64_t> completed_appends_tail_;  // Completed appends tail.
  ValueOffsets value_offsets_;                    // Lock-free, dynamically
                                                  // growing list of value
                                                  // offsets.
  ArrayNGramIdx<> ngram_idx_;                     // Lock-free dynamically
                                                  // Growing index mapping
                                                  // N-grams to their locations
                                                  // in the log.
};
}

#endif

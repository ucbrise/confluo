#ifndef SLOG_LOGSTORE_H_
#define SLOG_LOGSTORE_H_

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
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

#include "kvmap.h"
#include "ngram_idx.h"

namespace slog {

template<uint32_t MAX_KEYS = 134217728, uint32_t LOG_SIZE = UINT_MAX,
    class kvmap_type = sysgen_kvmap>
class log_store {
 public:
  // The internal key component of the tail increment for appends and updates.
  static const uint64_t KEY_INCR = 1ULL << 32;

  // The tail increment for a delete operation; we don't increment the internal
  // key, only the value offset by one byte.
  static const uint64_t DEL_INCR = 1ULL;

  // Constructor to initialize the LogStore.
  log_store() {
    // Initialize the log store to a constant size.

    // Note: we can use a lock-free exponentially or linear growing allocator
    // to make the Log dynamically sized rather than static.
    data_log_ = new char[LOG_SIZE];

    // Initialize both read and write tail to 0.
    write_tail_.store(0);
    read_tail_.store(0);

    index_log_ = new ngram_index;
  }

  // Adds a new key value pair to the LogStore atomically.
  //
  // Returns 0 for a successful append, -1 otherwise.
  bool insert(const int64_t key, const std::string& value) {
    // Add the value to the Log and generate and advance the ongoing
    // appends tail.
    uint64_t current_tail = internal_append(value);

    // Obtain the tail increment for the value.
    uint64_t tail_increment = increment_tail(value.length());

    // Add external to internal key mapping
    kvmap_.add_external(key, current_tail >> 32, current_tail & 0xFFFFFFFF);

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    atomic_advance_read_tail(current_tail, tail_increment);

    // Return true for success
    return true;
  }

  int64_t insert(const std::string& value) {
    // Add the value to the Log and generate and advance the ongoing
    // appends tail.
    uint64_t current_tail = internal_append(value);

    // Obtain the tail increment for the value.
    uint64_t tail_increment = increment_tail(value.length());

    // Add external to internal key mapping
    kvmap_.add_external(current_tail >> 32, current_tail >> 32,
                        current_tail & 0xFFFFFFFF);

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    atomic_advance_read_tail(current_tail, tail_increment);

    // Return internal key
    return current_tail >> 32;
  }

  // Internal append operation to add a value and its corresponding index
  // entries to the LogStore, and generate an internal key atomically.
  // Advances the ongoing appends tail, but does not update the completed
  // appends tail. This operation is always successful.
  //
  // Returns the ongoing appends tail that this append saw when it started.
  uint64_t internal_append(const std::string& value) {
    // Atomically update the ongoing append tail of the log
    uint64_t tail_increment = increment_tail(value.length());
    uint64_t current_tail = atomic_advance_write_tail(tail_increment);

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
    kvmap_.add_internal(internal_key, value_offset);

    // Similarly, we can append the value to the log without locking
    // since this thread has exclusive access to the region (value_offset, value_offset + .
    memcpy(data_log_ + value_offset, value.c_str(), value_length);

    // Safely update secondary index entries, in a lock-free manner.
    uint32_t value_end = value_offset + value_length;
    for (uint32_t i = value_offset; i <= value_end - NGRAM_N; i++)
      index_log_->add_offset(data_log_ + i, i);

    // Return the current tail
    return current_tail;
  }

  // Fetch a value from the LogStore by its internal key.
  //
  // Returns 0 if the fetch is successful, -1 otherwise.
  const bool get(char* value, const int64_t key) {
    // Get the current completed appends tail, and get the maximum valid key.
    uint64_t current_tail = read_tail_;
    uint32_t max_key = current_tail >> 32;
    uint32_t mark = current_tail & 0xFFFFFFFF;

    // Convert to internal key
    uint32_t internal_key;
    bool exists = kvmap_.ext_to_int(key, &internal_key);

    // If requested internal key is < max_key, the write
    // for the internal key hasn't completed yet. Return -1
    // indicating failure.
    if (!exists || internal_key >= max_key)
      return false;

    uint32_t start, end;
    bool valid = kvmap_.fwd_lookup_internal(internal_key, max_key, mark, &start,
                                            &end);

    // Return false if the forward mapping is invalid.
    if (!valid)
      return false;

    char* data_ptr = data_log_ + start;
    uint32_t i = 0;
    for (; i < end - start && data_ptr[i] != 0; i++)
      value[i] = data_ptr[i];
    value[i] = '\0';

    // Return true for successful get.
    return true;
  }

  // Search the LogStore for a query string.
  //
  // Returns the set of valid, matching internal keys.
  const void search(std::set<int64_t>& results, const std::string& query) {
    // Get the current completed appends tail, and extract the maximum valid
    // key and value offset.
    uint64_t current_tail = read_tail_;
    uint32_t max_key = current_tail >> 32;
    uint32_t max_off = current_tail & 0xFFFFFFFF;

    // Obtain the offsets into the values corresponding to the prefix and
    // suffix ngram for the substring from the N-gram index.
    char *substr = (char *) query.c_str();
    offset_list* prefix_offsets = index_log_->get_offsets(substr);
    offset_list* suffix_offsets = index_log_->get_offsets(
        substr + query.length() - NGRAM_N);

    if (prefix_offsets->size() < suffix_offsets->size()) {
      // Extract the remaining suffix to compare with the actual data.
      char *suffix = substr + NGRAM_N;
      size_t suffix_len = query.length() - NGRAM_N;

      // Scan through the list of offsets, adding only valid offsets into the
      // set of results.
      uint32_t size = prefix_offsets->size();
      char* data_ptr = data_log_ + NGRAM_N;
      for (uint32_t i = 0; i < size; i++) {
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
        uint32_t off = prefix_offsets->at(i);
        if (off < max_off && !strncmp(data_ptr + off, suffix, suffix_len))
          find_and_insert_key(results, off, max_key, max_off);
      }
    } else {
      // Extract the remaining prefix to compare with the actual data.
      char *prefix = substr;
      size_t prefix_len = query.length() - NGRAM_N;

      // Scan through the list of offsets, adding only valid offsets into the
      // set of results.
      uint32_t size = suffix_offsets->size();
      char* data_ptr = data_log_ - prefix_len;
      for (uint32_t i = 0; i < size; i++) {
        // An offset is valid if
        // (1) the remaining query prefix matches the data at that location in
        //     the log,
        // (2) the location does not exceed the maximum valid offset (i.e., the
        //     write at that location was incomplete when the search started)
        // (3) the key is not larger than the maximum valid key (i.e., the write
        //     for that key was incomplete when the search started)
        // (4) the key was not deleted before the search began.
        //
        // TODO: Take care of query.length() <= NGRAM_N case
        uint32_t off = suffix_offsets->at(i);
        if (off < max_off && !strncmp(data_ptr + off, prefix, prefix_len))
          find_and_insert_key(results, off, max_key, max_off);
      }
    }
  }

  // Search the LogStore for a column value.
  //
  // Returns the set of valid, matching internal keys.
  const void col_search(std::vector<int64_t>& results,
                        const std::string& col_value) {
    // Get the current completed appends tail, and extract the maximum valid
    // key and value offset.
    uint64_t current_tail = read_tail_;
    uint32_t max_key = current_tail >> 32;
    uint32_t max_off = current_tail & 0xFFFFFFFF;

    // Obtain the offsets into the values corresponding to the prefix and
    // suffix ngram for the substring from the N-gram index.
    char *substr = (char *) col_value.c_str();
    offset_list* prefix_offsets = index_log_->get_offsets(substr);
    offset_list* suffix_offsets = index_log_->get_offsets(
        substr + col_value.length() - NGRAM_N);

    uint32_t prefix_size = prefix_offsets->size();
    uint32_t suffix_size = suffix_offsets->size();
    if (prefix_size < suffix_size) {
      // Extract the remaining suffix to compare with the actual data.
      char *suffix = substr + NGRAM_N;
      size_t suffix_len = col_value.length() - NGRAM_N;

      // Scan through the list of offsets, adding only valid offsets into the
      // set of results.
      uint32_t size = prefix_offsets->size();
      char* data_ptr = data_log_ + NGRAM_N;
      for (uint32_t i = 0; i < size; i++) {
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
        uint32_t off = prefix_offsets->at(i);
        if (off < max_off && !strncmp(data_ptr + off, suffix, suffix_len))
          find_and_insert_key(results, off, max_key, max_off);
      }
    } else {
      // Extract the remaining prefix to compare with the actual data.
      char *prefix = substr;
      size_t prefix_len = col_value.length() - NGRAM_N;

      // Scan through the list of offsets, adding only valid offsets into the
      // set of results.
      uint32_t size = suffix_offsets->size();
      char* data_ptr = data_log_ - prefix_len;
      for (uint32_t i = 0; i < size; i++) {
        // An offset is valid if
        // (1) the remaining query prefix matches the data at that location in
        //     the log,
        // (2) the location does not exceed the maximum valid offset (i.e., the
        //     write at that location was incomplete when the search started)
        // (3) the key is not larger than the maximum valid key (i.e., the write
        //     for that key was incomplete when the search started)
        // (4) the key was not deleted before the search began.
        //
        // TODO: Take care of query.length() <= NGRAM_N case
        uint32_t off = suffix_offsets->at(i);
        if (off < max_off && !strncmp(data_ptr + off, prefix, prefix_len))
          find_and_insert_key(results, off, max_key, max_off);
      }
    }
  }

  // Atomically deletes the key from the LogStore.
  //
  // Returns true if the delete is successful, false if the key was already
  // deleted or not yet created.
  bool delete_record(const int64_t key) {
    uint32_t internal_key;
    if (!kvmap_.ext_to_int(key, &internal_key))
      return false;

    // Atomically increase the ongoing append tail of the log.
    uint64_t current_tail = atomic_advance_write_tail(DEL_INCR);

    // Obtain the offset into the Log corresponding to the current Log.
    uint32_t value_offset = current_tail & 0xFFFFFFFF;

    // Throw an exception if the delete causes the Log to grow beyond the
    // maximum Log size.
    if (value_offset + 1 >= LOG_SIZE)
      throw -1;

    if (internal_key >= current_tail >> 32)
      return false;

    // Invalidate the given internal key.
    bool success = kvmap_.invalidate_internal(internal_key, value_offset + 1);

    // Atomically update the completed append tail of the log.
    // This is done using CAS, and may have bounded waiting until
    // all appends before the current_tail are completed.
    atomic_advance_read_tail(current_tail, DEL_INCR);

    return success;
  }

  // Atomically get the number of currently readable keys.
  const uint32_t get_num_keys() {
    uint64_t current_tail = read_tail_;
    return current_tail >> 32;
  }

  // Atomically get the size of the currently readable portion of the LogStore.
  const uint32_t get_size() {
    uint64_t current_tail = read_tail_;
    return current_tail & 0xFFFFFFFF;
  }

  // Get the difference between the ongoing appends tail and the completed
  // appends tail. Note that the operation is not atomic, and should only be
  // used for approximate measurements.
  const uint64_t get_gap() {
    return write_tail_ - read_tail_;
  }

 private:
  // Compute the tail increment for a given value length.
  //
  // Returns the tail increment.
  uint64_t increment_tail(uint32_t value_length) {
    return KEY_INCR | value_length;
  }

  // Atomically advance the write tail by the given amount.
  //
  // Returns the tail value just before the advance occurred.
  uint64_t atomic_advance_write_tail(uint64_t tail_increment) {
    return std::atomic_fetch_add(&write_tail_, tail_increment);
  }

  // Atomically advance the read tail by the given amount.
  // Waits if there are appends before the expected append tail.
  void atomic_advance_read_tail(uint64_t expected_append_tail,
                                uint64_t tail_increment) {
    while (!std::atomic_compare_exchange_weak(
        &read_tail_, &expected_append_tail,
        expected_append_tail + tail_increment))
      ;
  }

  // Finds the internal key given data offset, the maximum valid key and offset,
  // and inserts the key into the provided set if it hasn't been deleted.
  const void find_and_insert_key(std::set<int64_t>& keys, const uint32_t offset,
                                 const uint32_t max_key,
                                 const uint32_t max_off) {

    uint32_t internal_key;
    bool valid = kvmap_.bwd_lookup_internal(offset, max_off, 0, max_key,
                                            &internal_key);

    int64_t external_key;
    if (valid && kvmap_.int_to_ext(internal_key, &external_key)) {
      keys.insert(external_key);
    }
  }

  // Finds the internal key given data offset, the maximum valid key and offset,
  // and inserts the key into the provided set if it hasn't been deleted.
  const void find_and_insert_key(std::vector<int64_t>& keys,
                                 const uint32_t offset, const uint32_t max_key,
                                 const uint32_t max_off) {
    uint32_t internal_key;
    bool valid = kvmap_.bwd_lookup_internal(offset, max_off, 0, max_key,
                                            &internal_key);

    int64_t external_key;
    if (valid && kvmap_.int_to_ext(internal_key, &external_key)) {
      keys.push_back(external_key);
    }
  }

  char *data_log_;                                 // Data log
  std::atomic<uint64_t> write_tail_;               // Write tail
  std::atomic<uint64_t> read_tail_;                // Read tail
  kvmap_type kvmap_;                               // Key-value offset mapping
  ngram_index *index_log_;                         // Index log
};
}

#endif /* SLOG_LOGSTORE_H_ */

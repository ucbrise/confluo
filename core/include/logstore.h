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

#include "indexlog.h"
#include "offsetlog.h"
#include "exceptions.h"

namespace slog {

struct tokens {
  unsigned char* time;
  unsigned char* src_ip;
  unsigned char* dst_ip;
  unsigned char* src_prt;
  unsigned char* dst_prt;
};

template<uint32_t LOG_SIZE = UINT_MAX>
class log_store {
 public:
  // The internal key component of the tail increment for appends and updates.
  static const uint64_t KEY_INCR = 1ULL << 32;

  // Constructor to initialize the LogStore.
  log_store() {
    // Initialize the log store to a constant size.

    // Note: we can use a lock-free exponentially or linear growing allocator
    // to make the Log dynamically sized rather than static.
    dlog_ = new char[LOG_SIZE];

    // Initialize data log tail to 1. Offset 0 is used to indicate invalid records.
    dtail_.store(1);

    time_idx_ = new indexlog<4, 3>();
    srcip_idx_ = new indexlog<4, 3>();
    dstip_idx_ = new indexlog<4, 3>();
    srcprt_idx_ = new indexlog<2, 2>();
    dstprt_idx_ = new indexlog<2, 2>();
  }

  uint64_t insert(const unsigned char* record, uint16_t record_len,
                  const tokens& tkns) {
    // Start the insertion by obtaining a record id from offset log
    uint64_t record_id = olog_.start();

    // Atomically update the tail of the log
    uint64_t offset = atomic_advance_tail(record_len);

    // Throw an exception if internal key greater than the largest valid
    // internal key or end of the value goes beyond maximum Log size.
    //
    // TODO: Replace data log with monolog structure, so that
    // log_overflow_exception never occurs.
    if (offset + record_len >= LOG_SIZE)
      throw log_overflow_exception();

    // Append the record value to data log
    append_record(record, record_len, offset);

    // Append the index entries to index logs
    append_tokens(record_id, tkns);

    olog_.end(record_id, offset, record_len);

    // Return record_id
    return record_id;
  }

  // Atomically fetch a record from the LogStore by its recordId. The record
  // buffer must be pre-allocated.
  //
  // Returns true if the fetch is successful, false otherwise.
  const bool get(unsigned char* record, const int64_t record_id) {
    // Checks if the record_id has been written yet, returns false on failure.
    if (!olog_.is_valid(record_id))
      return false;

    uint64_t offset;
    uint16_t length;
    olog_.lookup(record_id, offset, length);

    // Copy data from data log to record buffer.
    memcpy(record, dlog_ + offset, length);

    // Return true for successful get.
    return true;
  }

  // Atomically filter on time.
  const void filter_time(std::set<int64_t>& results,
                         const unsigned char* token_prefix,
                         const uint32_t token_prefix_len) {
    filter(time_idx_, results, token_prefix, token_prefix_len, olog_.num_ids());
  }

  // Atomically filter on src_ip.
  const void filter_src_ip(std::set<int64_t>& results,
                           const unsigned char* token_prefix,
                           const uint32_t token_prefix_len) {
    filter(srcip_idx_, results, token_prefix, token_prefix_len,
           olog_.num_ids());
  }

  // Atomically filter on dst_ip.
  const void filter_dst_ip(std::set<int64_t>& results,
                           const unsigned char* token_prefix,
                           const uint32_t token_prefix_len) {
    filter(dstip_idx_, results, token_prefix, token_prefix_len,
           olog_.num_ids());
  }

  // Atomically filter on src_port.
  const void filter_src_port(std::set<int64_t>& results,
                             const unsigned char* token_prefix,
                             const uint32_t token_prefix_len) {
    filter(srcprt_idx_, results, token_prefix, token_prefix_len,
           olog_.num_ids());
  }

  // Atomically filter on dst_port.
  const void filter_dst_port(std::set<int64_t>& results,
                             const unsigned char* token_prefix,
                             const uint32_t token_prefix_len) {
    filter(dstprt_idx_, results, token_prefix, token_prefix_len,
           olog_.num_ids());
  }

  // Atomically get the number of currently readable keys.
  const uint32_t num_ids() {
    return olog_.num_ids();
  }

  // Atomically get the size of the currently readable portion of the LogStore.
  const uint32_t size() {
    return dtail_.load();
  }

 private:
  // Atomically advance the write tail by the given amount.
  //
  // Returns the tail value just before the advance occurred.
  uint64_t atomic_advance_tail(uint64_t tail_increment) {
    return std::atomic_fetch_add(&dtail_, tail_increment);
  }

  // Append a (recordId, record) pair to the log store.
  void append_record(const unsigned char* record, uint16_t record_len,
                     uint32_t offset) {

    // We can append the value to the log without locking since this
    // thread has exclusive access to the region (offset, offset + record_len).
    memcpy(dlog_ + offset, record, record_len);
  }

  // Append (token, recordId) entries to index log.
  void append_tokens(uint32_t record_id, const tokens& tkns) {
    time_idx_->add_entry(tkns.time, record_id);
    srcip_idx_->add_entry(tkns.src_ip, record_id);
    dstip_idx_->add_entry(tkns.dst_ip, record_id);
    srcprt_idx_->add_entry(tkns.src_prt, record_id);
    dstprt_idx_->add_entry(tkns.dst_prt, record_id);
  }

  // Atomically filter recordIds on a given index log by given token prefix.
  //
  // Returns the set of valid, matching recordIds.
  template<uint32_t L1, uint32_t L2>
  const void filter(indexlog<L1, L2>* ilog, std::set<int64_t>& results,
                    const unsigned char* token_prefix,
                    const uint32_t token_prefix_len, const uint64_t max_rid) {

    if (L2 > token_prefix_len) {
      // Determine the range of prefixes in ilog
      unsigned char token_buf[L1];
      memcpy(token_buf, token_prefix, token_prefix_len);
      for (uint32_t token_idx = token_prefix_len; token_idx < L2; token_idx++) {
        token_buf[token_idx] = 0;
      }
      uint32_t start = token_ops<L1, L2>::prefix(token_prefix);
      for (uint32_t token_idx = token_prefix_len; token_idx < L2; token_idx++) {
        token_buf[token_idx] = 255;
      }
      uint32_t end = token_ops<L1, L2>::prefix(token_prefix);

      // Sweep through the range and return all matches
      for (uint32_t j = start; j <= end; j++) {
        // Don't need to check suffixes
        entry_list* list = ilog->get_entry_list(j);
        if (list == NULL)
          continue;
        uint32_t size = list->size();
        for (uint32_t i = 0; i < size; i++) {
          uint32_t record_id = list->at(i) & 0xFFFFFFFF;
          if (olog_.is_valid(record_id, max_rid))
            results.insert(record_id);
        }
      }
    } else {
      entry_list* list = ilog->get_entry_list(token_prefix);
      if (list == NULL)
        return;
      uint32_t size = list->size();
      if (L2 == token_prefix_len) {
        // Don't need to check suffixes
        for (uint32_t i = 0; i < size; i++) {
          uint32_t record_id = list->at(i) & 0xFFFFFFFF;
          if (olog_.is_valid(record_id, max_rid))
            results.insert(record_id);
        }
      } else {
        // Need to filter by suffixes
        uint32_t ignore = L1 - token_prefix_len;
        for (uint32_t i = 0; i < size; i++) {
          index_entry entry = list->at(i);
          uint32_t record_id = entry & 0xFFFFFFFF;
          uint32_t entry_suffix = entry >> 32;
          uint32_t query_suffix = token_ops<L1, L2>::suffix(token_prefix);
          if (entry_suffix >> ignore == query_suffix >> ignore
              && olog_.is_valid(record_id, max_rid))
            results.insert(record_id);
        }
      }
    }
  }

  // Data log and index log
  char *dlog_;                                 // Data log
  offsetlog olog_;                             // recordId-record offset mapping
  std::atomic<uint64_t> dtail_;                 // Data log tail

  // Index logs
  indexlog<4, 3> *time_idx_;
  indexlog<4, 3> *srcip_idx_;
  indexlog<4, 3> *dstip_idx_;
  indexlog<2, 2> *srcprt_idx_;
  indexlog<2, 2> *dstprt_idx_;
};
}

#endif /* SLOG_LOGSTORE_H_ */

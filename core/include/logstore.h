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
  unsigned char** tokens;
  size_t* token_lengths;
  size_t num_tokens;
};

struct handle {
 public:
  handle(log_store& handle, uint64_t request_batch_size = 256,
         uint64_t data_block_size = 256 * 64)
      : handle_(handle) {
    id_block_size_ = request_batch_size;
    remaining_ids_ = 0;
    cur_id_ = 0;

    data_block_size_ = data_block_size;
    remaining_bytes_ = 0;
    cur_offset_ = 0;
  }

  uint64_t insert(const unsigned char* record, uint16_t record_len,
                  const tokens& tkns) {
    if (remaining_ids_ == 0) {
      cur_id_ = handle_.olog_->request_id_block(id_block_size_);
      remaining_ids_ = id_block_size_;
    }

    if (remaining_bytes_ < record_len) {
      cur_offset_ = handle_.request_bytes(data_block_size_);
      remaining_bytes_ = data_block_size_;
    }

    handle_.append_record(record, record_len, cur_offset_);
    handle_.append_tokens(cur_id_, tkns);
    handle_.olog_->set(cur_id_, cur_offset_, record_len);
    handle_.olog_->end(cur_id_);
    remaining_ids_--;
    cur_offset_ += record_len;
    return ++cur_id_;
  }

  const bool get(unsigned char* record, const int64_t record_id) {
    return handle_.get(record, record_id);
  }

 private:
  uint64_t data_block_size_;
  uint64_t id_block_size_;

  uint64_t cur_id_;
  uint64_t remaining_ids_;

  uint64_t cur_offset_;
  uint64_t remaining_bytes_;

  log_store& handle_;
};

struct storage {
  uint64_t dlog_size;
  uint64_t olog_size;
  uint64_t* idx_sizes;
};

class log_store {
 public:
  friend class handle;

  handle* get_handle() {
    return new handle(*this);
  }

  // Constructor to initialize the LogStore.
  log_store() {
    dlog_ = new __monolog_linear_base< unsigned char>;
    olog_ = new offsetlog;

    dtail_.store(0);
  }

  /**
   * Insert a new record into the log-store.
   *
   * @param record The buffer containing record data.
   * @param record_len The length of the record.
   * @param tkns Tokens associated with the record.
   * @return The unique record id generated for the record.
   */
  uint64_t insert(const unsigned char* record, uint16_t record_len,
      const tokens& tkns) {
    /* Atomically update the tail of the log */
    uint64_t offset = request_bytes(record_len);

    /* Start the insertion by obtaining a record id from offset log */
    uint64_t record_id = olog_->start(offset, record_len);

    /* Append the record value to data log */
    append_record(record, record_len, offset);

    /* Append the index entries to index logs */
    append_tokens(record_id, tkns);

    olog_->end(record_id);

    /* Return record_id */
    return record_id;
  }

  /**
   * Atomically fetch a record from the log-store given its recordId. The
   * record buffer must be pre-allocated with sufficient size.
   *
   * @param record The (pre-allocated) record buffer.
   * @param record_id The id of the record being requested.
   * @return Returns true if the fetch is successful, false otherwise.
   */
  const bool get(unsigned char* record, const uint64_t record_id) {

    /* Checks if the record_id has been written yet, returns false on failure. */
    if (!olog_->is_valid(record_id)) return false;

    uint64_t offset;
    uint16_t length;
    olog_->lookup(record_id, offset, length);

    /* Copy data from data log to record buffer. */
    dlog_->read(offset, record, length);

    /* Return true for successful get. */
    return true;
  }

  /**
   * Atomically extract a portion of the record from the log-store given its
   * record-id, offset into the record and the number of bytes required. The
   * record buffer must be pre-allocated with sufficient size.
   *
   * @param record The (pre-allocated) record-buffer.
   * @param record_id The id of the record being requested.
   * @param offset The offset into the record to begin extracting.
   * @param length The number of bytes to extract.
   * @return Returns true if the extract is successful, false otherwise.
   */
  const bool extract(unsigned char* record, const uint64_t record_id,
      uint32_t offset, uint32_t& length) {

    /* Checks if the record_id has been written yet, returns false on failure. */
    if (!olog_->is_valid(record_id)) return false;

    uint64_t record_offset;
    uint16_t record_length;
    olog_->lookup(record_id, record_offset, record_length);

    /* Compute the minimum of requested length and available data */
    length = std::min(length, record_length - offset);

    /* Copy data from data log to record buffer. */
    dlog_->read(record_offset + offset, record, length);

    /* Return true for successful extract. */
    return true;
  }

  /** Atomically get the number of currently readable records.
   *
   * @return The number of readable records.
   */
  const uint64_t num_records() {
    return olog_->num_ids();
  }

  /** Atomically get the size of the currently readable portion of the log-store.
   *
   * @return The size in bytes of the currently readable portion of the log-store.
   */
  const uint64_t size() {
    return dtail_.load();
  }

  /** Get storage statistics
   *
   * @param st The storage structure which will be populated with storage
   * statistics at the end of the call.
   */
  void storage_footprint(storage& st) {
    st.dlog_size = dlog_->storage_size();
    st.olog_size = olog_->storage_size();
    /* TODO: Add index sizes */
  }

private:

  /**
   * Atomically advance the write tail by the given amount.
   *
   * @param record_size The number of bytes being requested.
   * @return Returns the tail value just before the advance occurred.
   */
  uint64_t request_bytes(uint64_t request_bytes) {
    return dtail_.fetch_add(request_bytes);
  }

  /**
   * Append a (recordId, record) pair to the log store.
   *
   * @param record The buffer containing record data.
   * @param record_len The length of the buffer.
   * @param offset The offset into the log where data should be written.
   */
  void append_record(const unsigned char* record, uint16_t record_len,
      uint64_t offset) {

    /* We can append the value to the log without locking since this
     * thread has exclusive access to the region (offset, offset + record_len).
     */
    dlog_->write(offset, record, record_len);
  }

  /**
   * Append (token, recordId) entries to index log.
   *
   * @param record_id The id of the record whose index entries are being added.
   * @param tkns The tokens associated with the record.
   */
  void append_tokens(uint64_t record_id, const tokens& tkns) {
    /* TODO: idx_->add_entry(tkns.tkn[i], record_id); */
  }

  /**
   * Atomically filter record ids from a given index for a token prefix.
   *
   * @param results The results set to be populated with the ids of matching records.
   * @param ilog The index log associated with the token.
   * @param token_prefix The prefix of the token being searched.
   * @param token_prefix_len The length of the prefix.
   * @param max_rid Largest record-id to consider.
   */
  template<uint32_t L1, uint32_t L2>
  const void filter(std::set<uint64_t>& results, indexlog<L1, L2>* ilog,
      const unsigned char* token_prefix,
      const uint32_t token_prefix_len, const uint64_t max_rid) {

    if (L2 > token_prefix_len) {
      /* Determine the range of prefixes in ilog */
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

      /* Sweep through the range and return all matches */
      for (uint32_t j = start; j <= end; j++) {
        /* Don't need to check suffixes */
        entry_list* list = ilog->get_entry_list(j);
        sweep_without_check(results, list, max_rid);
      }
    } else {
      entry_list* list = ilog->get_entry_list(token_prefix);
      if (L2 == token_prefix_len) {
        sweep_without_check(results, list, max_rid);
      } else {
        if (list == NULL) return;

        /* Need to filter by suffixes */
        uint32_t ignore = L1 - token_prefix_len;
        uint32_t size = list->size();
        for (uint32_t i = 0; i < list->size(); i++) {
          index_entry entry = list->at(i);
          uint32_t record_id = entry & 0xFFFFFFFF;
          uint32_t entry_suffix = entry >> 32;
          uint32_t query_suffix = token_ops<L1, L2>::suffix(token_prefix);
          if (entry_suffix >> ignore == query_suffix >> ignore
              && olog_->is_valid(record_id, max_rid))
          results.insert(record_id);
        }
      }
    }
  }

  void sweep_without_check(std::set<uint64_t>& results, entry_list* list,
      uint64_t max_rid) {

    if (list == NULL) return;

    uint32_t size = list->size();
    for (uint32_t i = 0; i < size; i++) {
      uint32_t record_id = list->at(i) & 0xFFFFFFFF;
      if (olog_->is_valid(record_id, max_rid))
      results.insert(record_id);
    }
  }

  /* Data log and offset log */
  __monolog_linear_base<unsigned char>* dlog_;
  offsetlog* olog_;

  /* Tail for preserving atomicity */
  std::atomic<uint64_t> dtail_;

  /* TODO: Index logs */

};
}

#endif /* SLOG_LOGSTORE_H_ */

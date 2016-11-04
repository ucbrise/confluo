#ifndef SLOG_LOGSTORE_H_
#define SLOG_LOGSTORE_H_

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cassert>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <string>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <atomic>

#include "tokens.h"
#include "tieredindex.h"
#include "streamlog.h"
#include "offsetlog.h"
#include "filterops.h"
#include "utils.h"
#include "exceptions.h"

#define OFFSETMIN 1024
#define OFFSET1 1024
#define OFFSET2 2048
#define OFFSET3 4096
#define OFFSET4 8192
#define OFFSET5 16384
#define OFFSET6 32768
#define OFFSET7 65536
#define OFFSET8 131072

namespace slog {

struct logstore_storage {
  size_t dlog_size;
  size_t olog_size;
  std::vector<size_t> idx_sizes;
  std::vector<size_t> stream_sizes;
};

class log_store {
 public:
  class handle {
   public:
    /**
     * Constructor to initialize handle.
     *
     * @param base The base log-store for the handle.
     * @param request_batch_size The record batch size (#records) for insert queries.
     * @param data_block_size The data block size (#bytes) for insert queries.
     */
    handle(log_store& base, uint64_t request_batch_size = 256,
           uint64_t data_block_size = 256 * 64)
        : base_(base) {
      id_block_size_ = request_batch_size;
      remaining_ids_ = 0;
      cur_id_ = 0;

      data_block_size_ = data_block_size;
      remaining_bytes_ = 0;
      cur_offset_ = 0;
    }

    /**
     * Add a new index of specified token-length and prefix-length.
     *
     * @param token_length Length of the tokens.
     * @param prefix_length Length of the prefix being indexed.
     *
     * @return The id (> 0) of the newly created index. Returns zero on failure.
     */
    uint32_t add_index(uint32_t token_length) {
      return base_.add_index(token_length);
    }

    /**
     * Add a new stream with a specified filter function.
     *
     * @param fn The filter function.
     * @return The id of the newly created stream.
     */
    uint32_t add_stream(filter_function fn) {
      return base_.add_stream(fn);
    }

    /**
     * Insert a new record into the log-store.
     *
     * @param record The buffer containing record data.
     * @param record_len The length of the record.
     * @param tokens Tokens associated with the record.
     * @return The unique record id generated for the record.
     */
    uint64_t insert(const unsigned char* record, uint16_t record_len,
                    token_list& tkns) {
      if (remaining_ids_ == 0) {
        cur_id_ = base_.olog_->request_id_block(id_block_size_);
        remaining_ids_ = id_block_size_;
      }

      if (remaining_bytes_ < record_len) {
        cur_offset_ = base_.request_bytes(data_block_size_);
        remaining_bytes_ = data_block_size_;
      }

      base_.append_record(record, record_len, cur_offset_);
      base_.update_indexes(cur_id_, tkns);
      base_.olog_->set(cur_id_, cur_offset_, record_len);
      base_.olog_->end(cur_id_);
      remaining_ids_--;
      cur_offset_ += record_len;
      return ++cur_id_;
    }

    /**
     * Atomically fetch a record from the log-store given its recordId. The
     * record buffer must be pre-allocated with sufficient size.
     *
     * @param record The (pre-allocated) record buffer.
     * @param record_id The id of the record being requested.
     * @return true if the fetch is successful, false otherwise.
     */
    const bool get(unsigned char* record, const uint64_t record_id) {
      return base_.get(record, record_id);
    }

    /**
     * Atomically extract a portion of the record from the log-store given its
     * record-id, offset into the record and the number of bytes required. The
     * record buffer must be pre-allocated with sufficient size.
     *
     * @param record The (pre-allocated) record-buffer.
     * @param record_id The id of the record being requested.
     * @param offset The offset into the record to begin extracting.
     * @param length The number of bytes to extract. Updated with the actual
     *  number of bytes extracted.
     * @return true if the extract is successful, false otherwise.
     */
    const bool extract(unsigned char* record, const uint64_t record_id,
                       uint32_t offset, uint32_t& length) {
      return base_.extract(record, record_id, offset, length);
    }

    /**
     * Filter index-entries based on query.
     *
     * @param results The results of the filter query.
     * @param query The filter query.
     */
    void filter(std::unordered_set<uint64_t>& results, filter_query& query) {
      base_.filter(results, query);
    }

    /**
     * Get the stream associated with a given stream id.
     *
     * @param stream_id The id of the stream.
     * @return The stream associated with the id.
     */
    entry_list* get_stream(uint32_t stream_id) {
      return base_.get_stream(stream_id);
    }

    /* Statistics and helpers */

    /** Atomically get the number of currently readable records.
     *
     * @return The number of readable records.
     */
    const uint64_t num_records() {
      return base_.num_records();
    }

    /** Atomically get the size of the currently readable portion of the log-store.
     *
     * @return The size in bytes of the currently readable portion of the log-store.
     */
    const uint64_t size() {
      return base_.size();
    }

    /** Get storage statistics
     *
     * @param storage_stats The storage structure which will be populated with
     * storage statistics at the end of the call.
     */
    void storage_footprint(logstore_storage& storage_stats) {
      base_.storage_footprint(storage_stats);
    }

   private:
    uint64_t data_block_size_;
    uint64_t id_block_size_;

    uint64_t cur_id_;
    uint64_t remaining_ids_;

    uint64_t cur_offset_;
    uint64_t remaining_bytes_;

    log_store& base_;
  };

  /**
   * Constructor to initialize the log-store.
   */
  log_store() {
    /* Initialize data log and offset log */
    dlog_ = new __monolog_linear_base <uint8_t>;
    olog_ = new offsetlog;

    /* Initialize data log tail to zero. */
    dtail_.store(0);

    /* Initialize all index classes */
    idx1_ = new monolog_linearizable<__index1 *>;
    idx2_ = new monolog_linearizable<__index2 *>;
    idx3_ = new monolog_linearizable<__index3 *>;
    idx4_ = new monolog_linearizable<__index4 *>;
    idx5_ = new monolog_linearizable<__index5 *>;
    idx6_ = new monolog_linearizable<__index6 *>;
    idx7_ = new monolog_linearizable<__index7 *>;
    idx8_ = new monolog_linearizable<__index8 *>;

    /* Initialize stream logs */
    streams_ = new monolog_linearizable<streamlog*>;
  }

  /**
   * Get a handle to the log-store.
   *
   * @return A handle to the log-store.
   */
  handle* get_handle() {
    return new handle(*this);
  }

  /**
   * Add a new index for tokens of specified length.
   *
   * @param token_length Length of the tokens to be indexed.
   *
   * @return The id (> 0) of the newly created index. Returns zero on failure.
   */
  uint32_t add_index(uint32_t token_length) {
    switch (token_length) {
      case 1:
        return OFFSET1 + idx1_->push_back(new __index1);
      case 2:
        return OFFSET2 + idx2_->push_back(new __index2);
      case 3:
        return OFFSET3 + idx3_->push_back(new __index3);
      case 4:
        return OFFSET4 + idx4_->push_back(new __index4);
      case 5:
        return OFFSET5 + idx5_->push_back(new __index5);
      case 6:
        return OFFSET6 + idx6_->push_back(new __index6);
      case 7:
        return OFFSET7 + idx7_->push_back(new __index7);
      case 8:
        return OFFSET8 + idx8_->push_back(new __index8);
    }

    return 0;
  }

  /**
   * Add a new stream with a specified filter function.
   *
   * @param fn The filter function.
   * @return The id of the newly created stream.
   */
  uint32_t add_stream(filter_function fn) {
    return streams_->push_back(new streamlog(fn));
  }

  /**
   * Insert a new record into the log-store.
   *
   * @param record The buffer containing record data.
   * @param record_len The length of the record.
   * @param tokens Tokens associated with the record.
   * @return The unique record id generated for the record.
   */
  uint64_t insert(const unsigned char* record, uint16_t record_len,
                  token_list& tokens) {
    /* Atomically request bytes at the end of data-log */
    uint64_t offset = request_bytes(record_len);

    /* Start the insertion by obtaining a record id from offset log */
    uint64_t record_id = olog_->start(offset, record_len);

    /* Append the record value to data log */
    append_record(record, record_len, offset);

    /* Add the index entries to index logs */
    update_indexes(record_id, tokens);

    /* Add the record entry to appropriate streams */
    update_streams(record_id, record, record_len, tokens);

    /* End the write operation; makes the record available for query */
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
   * @return true if the fetch is successful, false otherwise.
   */
  bool get(unsigned char* record, const uint64_t record_id) {

    /* Checks if the record_id has been written yet, returns false on failure. */
    if (!olog_->is_valid(record_id))
      return false;

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
   * @param length The number of bytes to extract. Updated with the actual
   *  number of bytes extracted.
   * @return true if the extract is successful, false otherwise.
   */
  bool extract(unsigned char* record, const uint64_t record_id, uint32_t offset,
               uint32_t& length) {

    /* Checks if the record_id has been written yet, returns false on failure. */
    if (!olog_->is_valid(record_id))
      return false;

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

  /**
   * Get the stream associated with a given stream id.
   *
   * @param stream_id The id of the stream.
   * @return The stream associated with the id.
   */
  entry_list* get_stream(uint32_t stream_id) {
    return streams_->at(stream_id)->get_stream();
  }

  /* Statistics and helpers */

  /** Atomically get the number of currently readable records.
   *
   * @return The number of readable records.
   */
  uint64_t num_records() {
    return olog_->num_ids();
  }

  /** Atomically get the size of the currently readable portion of the log-store.
   *
   * @return The size in bytes of the currently readable portion of the log-store.
   */
  uint64_t size() {
    return dtail_.load();
  }

  /**
   * Filter index-entries based on query.
   *
   * @param results The results of the filter query.
   * @param query The filter query.
   */
  void filter(std::unordered_set<uint64_t>& results, filter_query& query) {
    uint64_t max_rid = olog_->num_ids();
    for (filter_conjunction& conjunction : query) {
      std::unordered_set<uint64_t> conjunction_results;
      for (basic_filter& basic : conjunction) {
        /* Identify which index the filter is on */
        uint32_t idx = basic.index_id() / OFFSETMIN;
        uint32_t off = basic.index_id() % OFFSETMIN;

        /* Query relevant index */
        std::unordered_set<uint64_t> filter_res;
        switch (idx) {
          case 1: {
            filter(filter_res, idx1_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 2: {
            filter(filter_res, idx2_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 4: {
            filter(filter_res, idx3_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 8: {
            filter(filter_res, idx4_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 16: {
            filter(filter_res, idx5_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 32: {
            filter(filter_res, idx6_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 64: {
            filter(filter_res, idx7_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
          case 128: {
            filter(filter_res, idx8_->at(off), basic.token_beg(),
                   basic.token_end(), max_rid, conjunction_results);
            break;
          }
        }
        /* Stop this sequence of conjunctions if filter results are empty */
        if (filter_res.empty())
          break;

        conjunction_results = filter_res;
      }
      results.insert(conjunction_results.begin(), conjunction_results.end());
    }
  }

  /** Get storage statistics
   *
   * @param storage_stats The storage structure which will be populated with
   * storage statistics at the end of the call.
   */
  void storage_footprint(logstore_storage& storage_stats) {
    /* Get size for data-log and offset-log */
    storage_stats.dlog_size = dlog_->storage_size();
    storage_stats.olog_size = olog_->storage_size();

    /* Get size for index-logs */
    index_size(storage_stats.idx_sizes, idx1_);
    index_size(storage_stats.idx_sizes, idx2_);
    index_size(storage_stats.idx_sizes, idx3_);
    index_size(storage_stats.idx_sizes, idx4_);
    index_size(storage_stats.idx_sizes, idx5_);
    index_size(storage_stats.idx_sizes, idx6_);
    index_size(storage_stats.idx_sizes, idx7_);
    index_size(storage_stats.idx_sizes, idx8_);

    /* Get size of stream-logs */
    stream_size(storage_stats.stream_sizes);
  }

 private:

  /**
   * Atomically request bytes from the data-log.
   *
   * @param record_size The number of bytes being requested.
   * @return The offset within data-log of the offered bytes.
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
   * Add (token, recordId) entries to index logs.
   *
   * @param record_id The id of the record whose index entries are being added.
   * @param tokens The tokens associated with the record.
   */
  void update_indexes(uint64_t record_id, token_list& tokens) {
    for (token_t& token : tokens) {
      /* Identify which index token belongs to */
      uint32_t idx = token.index_id() / OFFSETMIN;
      uint32_t off = token.index_id() % OFFSETMIN;

      /* Update relevant index */
      uint64_t key = token.data();
      switch (idx) {
        case 1: {
          idx1_->at(off)->add_entry(key, record_id);
          break;
        }
        case 2: {
          idx2_->at(off)->add_entry(key, record_id);
          break;
        }
        case 4: {
          idx3_->at(off)->add_entry(key, record_id);
          break;
        }
        case 8: {
          idx4_->at(off)->add_entry(key, record_id);
          break;
        }
        case 16: {
          idx5_->at(off)->add_entry(key, record_id);
          break;
        }
        case 32: {
          idx6_->at(off)->add_entry(key, record_id);
          break;
        }
        case 64: {
          idx7_->at(off)->add_entry(key, record_id);
          break;
        }
        case 128: {
          idx8_->at(off)->add_entry(key, record_id);
          break;
        }
      }
    }
  }

  /**
   * Add recordId to all streams which are satisfied by the record.
   *
   * @param record_id Id of the record.
   * @param record Record data.
   * @param record_len Record data length.
   * @param tokens Tokens of the record.
   */
  void update_streams(uint64_t record_id, const unsigned char* record,
                      const uint16_t record_len, const token_list& tokens) {
    uint32_t num_streams = streams_->size();
    for (uint32_t i = 0; i < num_streams; i++) {
      streams_->at(i)->check_and_add(record_id, record, record_len, tokens);
    }
  }

  /**
   * Atomically filter record ids from a given index for a token prefix.
   *
   * @param results The results set to be populated with matching records ids.
   * @param ilog The index log associated with the token.
   * @param token_prefix The prefix of the token being searched.
   * @param token_prefix_len The length of the prefix.
   * @param max_rid Largest record-id to consider.
   * @param superset The superset to which the results must belong.
   */
  template<typename INDEX>
  const void filter(std::unordered_set<uint64_t>& results, INDEX* index,
                    uint64_t token_beg, uint64_t token_end,
                    const uint64_t max_rid,
                    const std::unordered_set<uint64_t>& superset) {

    for (uint64_t i = token_beg; i <= token_end; i++) {
      entry_list* list = index->get(i);
      sweep_list(results, list, max_rid, superset);
    }
  }

  /**
   * Sweeps through the entry-list, adding all valid entries to the results.
   *
   *
   * @param results The set of results to be populated.
   * @param list The entry list.
   * @param max_rid The maximum permissible record id.
   * @param superset The superset to which the results must belong.
   * @param superset_check Flag which determines whether to perform superset
   *  check or not.
   */
  const void sweep_list(std::unordered_set<uint64_t>& results, entry_list* list,
                        uint64_t max_rid,
                        const std::unordered_set<uint64_t>& superset) {
    if (list == NULL)
      return;

    uint32_t size = list->size();
    for (uint32_t i = 0; i < size; i++) {
      uint64_t record_id = list->at(i);
      if (olog_->is_valid(record_id, max_rid)
          && (superset.empty() || superset.find(record_id) != superset.end()))
        results.insert(record_id);
    }
  }

  /**
   * Compute the sizes of index-logs.
   *
   * @param sizes Vector to be populated with index sizes.
   * @param idx Monolog containing indexes.
   */
  template<typename INDEX>
  const void index_size(std::vector<size_t>& sizes,
                        monolog_linearizable<INDEX*> *idx) {
    uint32_t num_indexes = idx->size();
    for (uint32_t i = 0; i < num_indexes; i++) {
      sizes.push_back(idx->at(i)->storage_size());
    }
  }

  /**
   * Compute the sizes of all stream logs.
   *
   * @param sizes Vector to be populated with stream sizes.
   */
  const void stream_size(std::vector<size_t>& sizes) {
    uint32_t num_streams = streams_->size();
    for (uint32_t i = 0; i < num_streams; i++) {
      sizes.push_back(streams_->at(i)->get_stream()->storage_size());
    }
  }

  /* Data log and offset log */
  __monolog_linear_base <uint8_t>* dlog_;
  offsetlog* olog_;

  /* Tail for preserving atomicity */
  std::atomic<uint64_t> dtail_;

  /* Index logs */
  monolog_linearizable<__index1 *> *idx1_;
  monolog_linearizable<__index2 *> *idx2_;
  monolog_linearizable<__index3 *> *idx3_;
  monolog_linearizable<__index4 *> *idx4_;
  monolog_linearizable<__index5 *> *idx5_;
  monolog_linearizable<__index6 *> *idx6_;
  monolog_linearizable<__index7 *> *idx7_;
  monolog_linearizable<__index8 *> *idx8_;

  /* Stream logs */
  monolog_linearizable<streamlog*> *streams_;
};

}

#endif /* SLOG_LOGSTORE_H_ */

#ifndef SLOG_STREAMLOG_H_
#define SLOG_STREAMLOG_H_

#include "monolog.h"
#include "entrylist.h"
#include "tokens.h"

namespace slog {

/* Filter function on record data and tokens. If true, record is kept,
 * otherwise filtered out. */
typedef bool (*filter_function)(uint64_t&, const unsigned char*, const uint16_t, const token_list&);

/**
 * A stream-log stores record-ids for all records that satisfy the
 * stream-specific filter function.
 */
class streamlog {
 public:
  /**
   * Constructor to initialize stream-log.
   * @param fn Filter function to use for this stream.
   */
  streamlog(filter_function fn) {
    fn_ = fn;
    stream_log_ = new entry_list;
  }

  /**
   * Filters record using the filter function, and adds the record if the
   * function matches.
   *
   * @param record_id The id of the record.
   * @param record The record data.
   * @param record_len The record data length.
   * @param tokens The tokens for the record.
   */
  void check_and_add(uint64_t record_id, const unsigned char* record,
                     const uint16_t record_len, const token_list& tokens) {
    if (fn_(record_id, record, record_len, tokens)) {
      stream_log_->push_back(record_id);
    }
  }

  /**
   * Get the underlying stream.
   *
   * @return The underlying stream.
   */
  entry_list* get_stream() {
    return stream_log_;
  }

 private:
  /* Filter function */
  filter_function fn_;

  /* Stream monolog */
  entry_list* stream_log_;
};

}

#endif /* SLOG_STREAMLOG_H_ */

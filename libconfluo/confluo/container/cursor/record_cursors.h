#ifndef CONFLUO_CONTAINER_CURSOR_RECORD_CURSOR_H_
#define CONFLUO_CONTAINER_CURSOR_RECORD_CURSOR_H_

#include <unordered_set>

#include "batched_cursor.h"
#include "offset_cursors.h"
#include "schema/record.h"
#include "parser/expression_compiler.h"
#include "container/data_log.h"

namespace confluo {

typedef batched_cursor<record_t> record_cursor;

/**
 * A distinct record cursor
 */
class distinct_record_cursor : public record_cursor {
 public:
  /**
   * Initializes a distinct record cursor
   *
   * @param r_cursor A pointer to the record cursor
   * @param batch_size The number of records in the batch
   */
  distinct_record_cursor(std::unique_ptr<record_cursor> r_cursor,
                         size_t batch_size = 64)
      : record_cursor(batch_size),
        r_cursor_(std::move(r_cursor)) {
    init();
  }

  /**
   * Loads the next batch from the cursor
   *
   * @return The size of the batch
   */
  virtual size_t load_next_batch() override {
    size_t i = 0;
    for (; i < current_batch_.size() && r_cursor_->has_more();
        ++i, r_cursor_->advance()) {
      record_t const& rec = r_cursor_->get();
      if (seen_.find(rec.log_offset()) == seen_.end()) {
        current_batch_[i] = rec;
        seen_.insert(rec.log_offset());
      } else {
        --i;
      }
    }
    return i;
  }

 private:
  std::unordered_set<size_t> seen_;
  std::unique_ptr<record_cursor> r_cursor_;
};

/**
 * Makes a distinct record cursor
 *
 * @param r_cursor A pointer to the record cursor
 * @param batch_size The number of records in the batch
 *
 * @return A pointer to the record cursor
 */
std::unique_ptr<record_cursor> make_distinct(
    std::unique_ptr<record_cursor> r_cursor, size_t batch_size = 64) {
  return std::unique_ptr<record_cursor>(
      new distinct_record_cursor(std::move(r_cursor), batch_size));
}

/**
 * A record cursor that filters out records
 */
class filter_record_cursor : public record_cursor {
 public:
  /**
   * Initializes the filter record
   *
   * @param o_cursor The offset cursor
   * @param dlog The data log pointer
   * @param schema The schema
   * @param cexpr The filter expression
   * @param batch_size The number of records in the batch
   */
  filter_record_cursor(std::unique_ptr<offset_cursor> o_cursor,
                       const data_log* dlog, const schema_t* schema,
                       const parser::compiled_expression& cexpr,
                       size_t batch_size = 64)
      : record_cursor(batch_size),
        o_cursor_(std::move(o_cursor)),
        dlog_(dlog),
        schema_(schema),
        cexpr_(cexpr) {
    init();
  }

  /**
   * Loads the next batch from the cursor
   *
   * @return The size of the batch
   */
  virtual size_t load_next_batch() override {
    size_t i = 0;
    for (; i < current_batch_.size() && o_cursor_->has_more();
        ++i, o_cursor_->advance()) {
      uint64_t o = o_cursor_->get();
      read_only_data_log_ptr ptr;
      dlog_->cptr(o, ptr);
      if (!cexpr_.test(current_batch_[i] = schema_->apply(o, ptr))) {
        i--;
      }
    }
    return i;
  }

 private:
  std::unique_ptr<offset_cursor> o_cursor_;
  const data_log* dlog_;
  const schema_t* schema_;
  const parser::compiled_expression& cexpr_;
};

}

#endif /* CONFLUO_CONTAINER_CURSOR_RECORD_CURSOR_H_ */

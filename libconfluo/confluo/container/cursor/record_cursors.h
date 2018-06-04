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
  distinct_record_cursor(std::unique_ptr<record_cursor> r_cursor, size_t batch_size = 64);

  /**
   * Loads the next batch from the cursor
   *
   * @return The size of the batch
   */
  virtual size_t load_next_batch() override;

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
std::unique_ptr<record_cursor> make_distinct(std::unique_ptr<record_cursor> r_cursor, size_t batch_size = 64);

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
                       const data_log *dlog, const schema_t *schema,
                       const parser::compiled_expression &cexpr,
                       size_t batch_size = 64);

  /**
   * Loads the next batch from the cursor
   *
   * @return The size of the batch
   */
  virtual size_t load_next_batch() override;

 private:
  std::unique_ptr<offset_cursor> o_cursor_;
  const data_log *dlog_;
  const schema_t *schema_;
  const parser::compiled_expression &cexpr_;
};

}

#endif /* CONFLUO_CONTAINER_CURSOR_RECORD_CURSOR_H_ */

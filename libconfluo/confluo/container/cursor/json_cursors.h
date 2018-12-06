#ifndef CONFLUO_CONTAINER_CURSOR_JSON_CURSOR_H_
#define CONFLUO_CONTAINER_CURSOR_JSON_CURSOR_H_

#include <unordered_set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "batched_cursor.h"
#include "offset_cursors.h"
#include "schema/record.h"
#include "parser/expression_compiler.h"
#include "container/data_log.h"

namespace confluo {

typedef batched_cursor<std::string> json_cursor;

/**
 * A json cursor that make records into a json formatted string
 */
class json_string_cursor : public json_cursor {
 public:
  /**
   * Initializes the filter record
   *
   * @param r_cursor The record cursor
   * @param schema The schema
   * @param batch_size The number of records in the batch
   */
  json_string_cursor(std::unique_ptr<record_cursor> r_cursor, const schema_t *schema,
                       size_t batch_size = 64);

  /**
   * Loads the next batch from the cursor
   *
   * @return The size of the batch
   */
  virtual size_t load_next_batch() override;

 private:
  std::unique_ptr<record_cursor> r_cursor_;
  const schema_t *schema_;
};

}

#endif /* CONFLUO_CONTAINER_CURSOR_JSON_CURSOR_H_ */

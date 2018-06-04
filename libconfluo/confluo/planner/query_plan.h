#ifndef CONFLUO_PLANNER_QUERY_PLAN_H_
#define CONFLUO_PLANNER_QUERY_PLAN_H_

#include <types/numeric.h>
#include <aggregate/aggregate_ops.h>
#include "container/lazy/stream.h"
#include "container/cursor/offset_cursors.h"
#include "container/cursor/record_cursors.h"
#include "parser/expression_compiler.h"
#include "query_ops.h"
#include "exceptions.h"

namespace confluo {
namespace planner {

/**
 * Container for a list of query_ops
 */
class query_plan : public std::vector<std::shared_ptr<query_op>> {
 public:
  /**
   * Initializes the query plan
   *
   * @param dlog The data log
   * @param schema The schema for the query plan
   * @param expr The query plan expression
   */
  query_plan(const data_log *dlog, const schema_t *schema, const parser::compiled_expression &expr);

  /**
   * Gets a string representation of the query plan
   *
   * @return std::string
   */
  std::string to_string();

  /**
   * Determines whether the execution will be optimized
   * @return True if a full scan is not required, false otherwise
   */
  bool is_optimized();

  /**
   * Executes the query plan 
   *
   * @param version The version of the multilog
   *
   * @return The pointer to the result of the query plan execution
   */
  std::unique_ptr<record_cursor> execute(uint64_t version);

  /**
   * Gets the aggregate for the query plan
   *
   * @param version The version of the atomic multilog
   * @param field_idx The field index
   * @param agg The aggregator for the aggregate
   *
   * @return The aggregate numeric
   */
  numeric aggregate(uint64_t version, uint16_t field_idx, const aggregator &agg);

 private:
  /**
   * Executes the query plan using full scan
   * @param version Version limit for execution
   * @return A record cursor over matching records
   */
  std::unique_ptr<record_cursor> using_full_scan(uint64_t version);

  /**
   * Executes the query plan using indexes
   * @param version Version limit for execution
   * @return A record cursor over matching records
   */
  std::unique_ptr<record_cursor> using_indexes(uint64_t version);

  const data_log *dlog_;
  const schema_t *schema_;
  const parser::compiled_expression &expr_;
};

}
}

#endif /* CONFLUO_PLANNER_QUERY_PLAN_H_ */

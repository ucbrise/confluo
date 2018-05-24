#ifndef CONFLUO_PLANNER_QUERY_PLANNER_H_
#define CONFLUO_PLANNER_QUERY_PLANNER_H_

#include <unordered_map>
#include <memory>

#include "container/radix_tree.h"
#include "exceptions.h"
#include "index_log.h"
#include "query_ops.h"
#include "query_plan.h"

namespace confluo {
namespace planner {

/**
 * Query planner class. Contains operations to map operations onto the
 * data.
 */
class query_planner {
 public:
  /** Range of keys */
  typedef std::pair<byte_string, byte_string> key_range;
  /** Maps an id to a key range */
  typedef std::map<uint32_t, key_range> key_range_map;
  /** List of index operation that can be performed */
  typedef std::vector<index_op> index_ops;
  /** Iterator through the index operations */
  typedef index_ops::iterator if_iterator;
  /** Constant iterator through the list of index operations */
  typedef index_ops::const_iterator const_if_iterator;

  /**
   * Initializes query_planner with given references to a data_log,
   * index_log, and schema
   * @param dlog A pointer to a data_log
   * @param idx_list A pointer to an index_log
   * @param schema A pointer to the schema
   */
  query_planner(const data_log* dlog, const index_log* idx_list, const schema_t* schema);

  /**
   * Converts a compiled_expression to a list of query_ops
   * @param expr the compiled expression
   * @throws illegal_state_exception
   * @return the query plan that contains a list of query_ops
   */
  query_plan plan(const parser::compiled_expression& expr) const;

 private:
  /**
   * Merges two key ranges together
   *
   * @param r1 The first key range
   * @param r2 The second key range
   *
   * @return A key range containing the combination of the given key ranges
   */
  key_range merge_range(const key_range& r1, const key_range& r2) const;

  /**
   * Adds a key range to the given key range map
   *
   * @param ranges The key range map that the range is added to
   * @param id The identifier of the key range
   * @param r The key range to add to the key range map
   *
   * @return True if the range was successfully added, false otherwise
   */
  bool add_range(key_range_map& ranges, uint32_t id, const key_range& r) const;

  /**
   * Optimizes the compiled minterm expression using the key ranges
   *
   * @param m The minterm to optimize
   *
   * @return Pointer to the optimized query operation
   */
  std::shared_ptr<query_op> optimize_minterm(const parser::compiled_minterm& m) const;

  const data_log* dlog_;
  const index_log* idx_list_;
  const schema_t* schema_;
};

}
}

#endif /* CONFLUO_PLANNER_QUERY_PLANNER_H_ */

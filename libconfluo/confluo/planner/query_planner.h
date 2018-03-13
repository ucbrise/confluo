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
  query_planner(const data_log* dlog, const index_log* idx_list,
                const schema_t* schema)
      : dlog_(dlog),
        idx_list_(idx_list),
        schema_(schema) {
  }

  /**
   * Converts a compiled_expression to a list of query_ops
   * @param expr the compiled expression
   * @throws illegal_state_exception
   * @return the query plan that contains a list of query_ops
   */
  query_plan plan(const compiled_expression& expr) const {
    query_plan qp(dlog_, schema_, expr);
    for (const compiled_minterm& m : expr) {
      std::shared_ptr<query_op> op = optimize_minterm(m);
      switch (op->op_type()) {
        case query_op_type::D_NO_OP:
          break;
        case query_op_type::D_NO_VALID_INDEX_OP: {
          qp.clear();
          qp.push_back(std::make_shared<full_scan_op>());
          return qp;
        }
        case query_op_type::D_INDEX_OP:
          qp.push_back(op);
          break;
        default:
          throw illegal_state_exception("Minterm generated invalid query_op");
      }
    }
    return qp;
  }

 private:
  /**
   * Merges two key ranges together
   *
   * @param r1 The first key range
   * @param r2 The second key range
   *
   * @return A key range containing the combination of the given key ranges
   */
  key_range merge_range(const key_range& r1, const key_range& r2) const {
    return std::make_pair(std::max(r1.first, r2.first),
                          std::min(r1.second, r2.second));
  }

  /**
   * Adds a key range to the given key range map
   *
   * @param ranges The key range map that the range is added to
   * @param id The identifier of the key range
   * @param r The key range to add to the key range map
   *
   * @return True if the range was successfully added, false otherwise
   */
  bool add_range(key_range_map& ranges, uint32_t id, const key_range& r) const {
    key_range_map::iterator it;
    key_range r_m;
    if ((it = ranges.find(id)) != ranges.end()) {  // Multiple key ranges
      r_m = merge_range(it->second, r);
    } else {  // Single key-range
      r_m = r;
    }

    if (r_m.first <= r_m.second) {  // Valid key range
      ranges[id] = r_m;
      return true;
    }
    return false;  // Invalid key-range
  }

  /**
   * Optimizes the compiled minterm expression using the key ranges
   *
   * @param m The minterm to optimize
   *
   * @return Pointer to the optimized query operation
   */
  std::shared_ptr<query_op> optimize_minterm(const compiled_minterm& m) const {
    // Get valid, condensed key-ranges for indexed attributes
    key_range_map m_key_ranges;
    for (const auto& p : m) {
      uint32_t idx = p.field_idx();
      const auto& col = (*schema_)[idx];
      if (col.is_indexed() && p.op() != reational_op_id::NEQ) {
        double bucket_size = col.index_bucket_size();
        key_range r;
        switch (p.op()) {
          case reational_op_id::EQ: {
            r = std::make_pair(p.value().to_key(bucket_size),
                               p.value().to_key(bucket_size));
            break;
          }
          case reational_op_id::GE: {
            r = std::make_pair(p.value().to_key(bucket_size),
                               col.max().to_key(bucket_size));
            break;
          }
          case reational_op_id::LE: {
            r = std::make_pair(col.min().to_key(bucket_size),
                               p.value().to_key(bucket_size));
            break;
          }
          case reational_op_id::GT: {
            r = std::make_pair(++p.value().to_key(bucket_size),
                               col.max().to_key(bucket_size));
            break;
          }
          case reational_op_id::LT: {
            r = std::make_pair(col.min().to_key(bucket_size),
                               --p.value().to_key(bucket_size));
            break;
          }
          default: {
            throw invalid_operation_exception("Invalid operator in predicate");
          }
        }

        if (!add_range(m_key_ranges, col.index_id(), r)) {
          return std::make_shared<no_op>();
        }
      }
    }

    if (m_key_ranges.empty()) {  // None of the fields are indexed
      return std::make_shared<no_valid_index_op>();
    }

    // If we've reached here, we only have non-zero valid, indexed key-ranges.
    // Now we only need to return the minimum cost index lookup
    uint32_t min_id;
    size_t min_cost = UINT64_MAX;
    for (const auto& m_entry : m_key_ranges) {
      size_t cost;
      // TODO: Make the cost function pluggable
      if ((cost = idx_list_->at(m_entry.first)->approx_count(
          m_entry.second.first, m_entry.second.second)) < min_cost) {
        min_cost = cost;
        min_id = m_entry.first;
      }
    }

    return std::make_shared<index_op>(idx_list_->at(min_id),
                                      m_key_ranges[min_id]);
  }

  const data_log* dlog_;
  const index_log* idx_list_;
  const schema_t* schema_;
};

}
}

#endif /* CONFLUO_PLANNER_QUERY_PLANNER_H_ */

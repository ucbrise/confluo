#ifndef DIALOG_QUERY_PLANNER_H_
#define DIALOG_QUERY_PLANNER_H_

#include <unordered_map>
#include <memory>

#include "radix_tree.h"
#include "index_log.h"
#include "exceptions.h"
#include "planner/query_ops.h"
#include "planner/query_plan.h"

// FIXME: A minterm does not yield any results if:
// (1) no index is present for any attribute
// (2) the only indexed attribute has a != relational operation

namespace dialog {
namespace planner {

class query_planner {
 public:
  typedef std::pair<byte_string, byte_string> key_range;
  typedef std::map<uint32_t, key_range> key_range_map;
  typedef std::vector<index_op> index_ops;
  typedef index_ops::iterator if_iterator;
  typedef index_ops::const_iterator const_if_iterator;

  query_planner(const compiled_expression& expr, const data_log& dlog,
                const index_log& idx_list, const schema_t& schema)
      : expr_(expr),
        dlog_(dlog),
        idx_list_(idx_list),
        schema_(schema) {
  }

  query_plan plan() {
    query_plan qp;
    for (const compiled_minterm& m : expr_) {
      std::shared_ptr<query_op> op = optimize_minterm(m);
      switch (op->op_type()) {
        case query_op_type::D_NO_OP:
          break;
        case query_op_type::D_NO_VALID_INDEX_OP: {
          qp.clear();
          qp.push_back(std::make_shared<full_scan_op>(full_scan_op(expr_)));
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
  key_range merge_range(const key_range& r1, const key_range& r2) {
    return std::make_pair(std::max(r1.first, r2.first),
                          std::min(r1.second, r2.second));
  }

  bool add_range(key_range_map& ranges, uint32_t id, const key_range& r) {
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

  std::shared_ptr<query_op> optimize_minterm(const compiled_minterm& m) {
    // Get valid, condensed key-ranges for indexed attributes
    key_range_map m_key_ranges;
    for (const auto& p : m) {
      uint32_t idx = p.field_idx();
      const auto& col = schema_[idx];
      if (col.is_indexed() && p.op() != relop_id::NEQ) {
        double bucket_size = col.index_bucket_size();
        key_range r;
        switch (p.op()) {
          case relop_id::EQ: {
            r = std::make_pair(p.value().to_key(bucket_size),
                               p.value().to_key(bucket_size));
            break;
          }
          case relop_id::GE: {
            r = std::make_pair(p.value().to_key(bucket_size),
                               col.max().to_key(bucket_size));
            break;
          }
          case relop_id::LE: {
            r = std::make_pair(col.min().to_key(bucket_size),
                               p.value().to_key(bucket_size));
            break;
          }
          case relop_id::GT: {
            r = std::make_pair(++p.value().to_key(bucket_size),
                               col.max().to_key(bucket_size));
            break;
          }
          case relop_id::LT: {
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
      if ((cost = idx_list_.at(m_entry.first)->approx_count(
          m_entry.second.first, m_entry.second.second)) < min_cost) {
        min_cost = cost;
        min_id = m_entry.first;
      }
    }

    return std::make_shared<index_op>(dlog_, idx_list_.at(min_id), schema_,
                                      m_key_ranges[min_id], m);
  }

//  size_t min_filter(const index_ops& filters) {
//    if (filters.size() == 1) {
//      return 0;
//    }
//
//    size_t min_idx = 0;
//    size_t min_count = UINT64_MAX;
//
//    for (size_t i = 0; i < filters.size(); i++) {
//      uint64_t count;
//      if ((count = idx_list_.at(filters.at(i).index_id())->approx_count(
//          filters.at(i).kbegin(), filters.at(i).kend()))) {
//        min_count = count;
//        min_idx = i;
//      }
//    }
//
//    return min_idx;
//  }
//
//  index_ops get_filters(const compiled_minterm& m) {
//    index_ops filters;
//    for (const compiled_predicate& p : m) {
//      const column_t& col = schema_[p.field_idx()];
//      if (col.is_indexed()) {
//        uint32_t id = col.index_id();
//        double bucket_size = col.index_bucket_size();
//        switch (p.op()) {
//          case relop_id::EQ: {
//            auto val = p.value().to_key(bucket_size);
//            filters.push_back(index_op(id, val, val));
//            break;
//          }
//          case relop_id::NEQ: {
//            auto min = col.min().to_key(bucket_size);
//            auto max = col.max().to_key(bucket_size);
//            auto val1 = (--p.value().to_key(bucket_size));
//            auto val2 = (++p.value().to_key(bucket_size));
//            filters.push_back(index_op(id, min, val1));
//            filters.push_back(index_op(id, val2, min));
//            break;
//          }
//          case relop_id::GE: {
//            auto val1 = p.value().to_key(bucket_size);
//            auto max = col.max().to_key(bucket_size);
//            filters.push_back(index_op(id, val1, max));
//            break;
//          }
//          case relop_id::LE: {
//            auto val1 = p.value().to_key(bucket_size);
//            auto min = col.min().to_key(bucket_size);
//            filters.push_back(index_op(id, min, val1));
//            break;
//          }
//          case relop_id::GT: {
//            auto val1 = (++p.value().to_key(bucket_size));
//            auto max = col.max().to_key(bucket_size);
//            filters.push_back(index_op(id, val1, max));
//            break;
//          }
//          case relop_id::LT: {
//            auto val1 = (--p.value().to_key(bucket_size));
//            auto min = col.min().to_key(bucket_size);
//            filters.push_back(index_op(id, min, val1));
//            break;
//          }
//          default: {
//            throw invalid_operation_exception(
//                "Invalid operator for index filter");
//          }
//        }
//      }
//    }
//    return filters;
//  }
//
//  index_ops reduce_filters(const index_ops& filters) {
//    index_ops out;
//    std::unordered_map<uint32_t, std::vector<size_t>> filter_map;
//    for (size_t i = 0; i < filters.size(); i++)
//      filter_map[filters.at(i).index_id()].push_back(i);
//
//    for (auto entry : filter_map) {
//      auto kbegin = filters[entry.second[0]].kbegin();
//      auto kend = filters[entry.second[0]].kend();
//      for (size_t j = 1; j < entry.second.size(); j++) {
//        kbegin =
//            kbegin > filters[entry.second[j]].kbegin() ?
//                kbegin : filters[entry.second[j]].kbegin();
//        kend =
//            kend < filters[entry.second[j]].kend() ?
//                kend : filters[entry.second[j]].kend();
//      }
//      if (kbegin <= kend)
//        out.push_back(index_op(entry.first, kbegin, kend));
//      else
//        return {};
//    }
//    return out;
//  }

  compiled_expression expr_;
  const data_log& dlog_;
  const index_log& idx_list_;
  const schema_t& schema_;
};

}
}

#endif /* DIALOG_QUERY_PLANNER_H_ */

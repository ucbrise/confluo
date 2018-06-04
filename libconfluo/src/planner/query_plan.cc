#include "planner/query_plan.h"

namespace confluo {
namespace planner {

query_plan::query_plan(const data_log *dlog, const schema_t *schema, const parser::compiled_expression &expr)
    : std::vector<std::shared_ptr<query_op>>(),
      dlog_(dlog),
      schema_(schema),
      expr_(expr) {}

std::string query_plan::to_string() {
  if (!is_optimized()) {
    return at(0)->to_string() + "(" + expr_.to_string() + ")";
  }
  std::string ret = "union(\n";
  for (auto &op : *this) {
    ret += "\t" + op->to_string() + ",\n";
  }
  ret += ").filter(" + expr_.to_string() + ")";
  return ret;
}

bool query_plan::is_optimized() {
  return !(size() == 1 && at(0)->op_type() == query_op_type::D_SCAN_OP);
}

std::unique_ptr<record_cursor> query_plan::execute(uint64_t version) {
  return is_optimized() ? using_indexes(version) : using_full_scan(version);
}

numeric query_plan::aggregate(uint64_t version, uint16_t field_idx, const aggregator &agg) {
  std::unique_ptr<record_cursor> cursor = execute(version);
  numeric accum = agg.zero;
  while (cursor->has_more()) {
    accum = agg.seq_op(accum, numeric(cursor->get()[field_idx].value()));
  }
  return accum;
}

std::unique_ptr<record_cursor> query_plan::using_full_scan(uint64_t version) {
  std::unique_ptr<offset_cursor> o_cursor(new data_log_cursor(version, schema_->record_size()));
  return std::unique_ptr<record_cursor>(new filter_record_cursor(std::move(o_cursor), dlog_, schema_, expr_));
}

std::unique_ptr<record_cursor> query_plan::using_indexes(uint64_t version) {
  if (size() == 1) {
    index::radix_index::rt_result ret = std::dynamic_pointer_cast<index_op>(at(0))->query_index();
    std::unique_ptr<offset_cursor>
        o_cursor(new offset_iterator_cursor<index::radix_index::rt_result::iterator>(ret.begin(), ret.end(), version));
    return std::unique_ptr<record_cursor>(new filter_record_cursor(std::move(o_cursor), dlog_, schema_, expr_));
  }
  std::vector<index::radix_index::rt_result> res;
  for (size_t i = 0; i < size(); i++) {
    res.push_back(std::dynamic_pointer_cast<index_op>(at(i))->query_index());
  }
  flattened_container<std::vector<index::radix_index::rt_result>> c(res);
  typedef flattened_container<std::vector<index::radix_index::rt_result>>::iterator iterator_t;
  std::unique_ptr<offset_cursor> o(new offset_iterator_cursor<iterator_t>(c.begin(), c.end(), version));
  return make_distinct(std::unique_ptr<record_cursor>(new filter_record_cursor(std::move(o), dlog_, schema_, expr_)));
}

}
}
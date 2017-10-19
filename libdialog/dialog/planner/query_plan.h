#ifndef DIALOG_QUERY_PLAN_H_
#define DIALOG_QUERY_PLAN_H_

#include "parser/expression_compiler.h"
#include "planner/query_ops.h"

namespace dialog {
namespace planner {

class query_plan : public std::vector<std::shared_ptr<query_op>> {
 public:
  typedef index_op::filtered_record_stream_t filtered_record_stream_t;
  typedef union_record_stream<filtered_record_stream_t> union_record_stream_t;

  std::string to_string() {
    std::string ret = "union(\n";
    for (auto& op : *this) {
      ret += "\t" + op->to_string() + ",\n";
    }
    ret += ")";
    return ret;
  }

  union_record_stream_t execute(uint64_t version) {
    // Our plan either contains a single full_scan_op, or a vector of index_ops
    // TODO: handle scan case, i.e., when
    //    size() == 1 && at(0)->op_type() == query_op_type::D_SCAN_OP
    std::vector<index_op::filtered_record_stream_t> streams;
    for (auto& op : *this) {
      auto idx_op = std::dynamic_pointer_cast<index_op>(op);
      streams.push_back(idx_op->execute(version));
    }
    return union_record_stream_t(streams);
  }
};

}
}

#endif /* DIALOG_QUERY_PLAN_H_ */

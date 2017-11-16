#ifndef DIALOG_QUERY_PLAN_H_
#define DIALOG_QUERY_PLAN_H_

#include "lazy/stream.h"
#include "parser/expression_compiler.h"
#include "planner/query_ops.h"

namespace confluo {
namespace planner {

class query_plan : public std::vector<std::shared_ptr<query_op>> {
 public:
  std::string to_string() {
    std::string ret = "union(\n";
    for (auto& op : *this) {
      ret += "\t" + op->to_string() + ",\n";
    }
    ret += ")";
    return ret;
  }

  bool is_optimized() {
    return !(size() == 1 && at(0)->op_type() == query_op_type::D_SCAN_OP);
  }

  lazy::stream<record_t> execute(uint64_t version) {
    return is_optimized() ? using_indexes(version) : using_full_scan(version);
  }

 private:
  lazy::stream<record_t> using_full_scan(uint64_t version) {
    return std::dynamic_pointer_cast<full_scan_op>(at(0))->execute(version);
  }

  lazy::stream<record_t> using_indexes(uint64_t version) {
    if (size() == 1) {
      return std::dynamic_pointer_cast<index_op>(at(0))->execute(version);
    }
    auto executor = [version](std::shared_ptr<query_op> op) {
      return std::dynamic_pointer_cast<index_op>(op)->execute(version);
    };
    return lazy::container_to_stream(*this).flat_map(executor).distinct();
  }
};

}
}

#endif /* DIALOG_QUERY_PLAN_H_ */

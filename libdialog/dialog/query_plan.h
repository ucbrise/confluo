#ifndef DIALOG_QUERY_PLAN_H_
#define DIALOG_QUERY_PLAN_H_

#include "compiled_minterm.h"

namespace dialog {

class minterm_plan {
 public:
  minterm_plan(const index_filter& ifilter, const compiled_minterm& dfilter)
      : idxf_(ifilter) {
    dataf_.insert(dfilter);
  }

  minterm_plan(const minterm_plan& other)
      : idxf_(other.idxf_),
        dataf_(other.dataf_) {
  }

  index_filter const& idx_filter() const {
    return idxf_;
  }

  compiled_expression const& data_filter() const {
    return dataf_;
  }

  std::string to_string() const {
    return idxf_.to_string() + " + filter(" + dataf_.to_string() + ")";
  }

 private:
  index_filter idxf_;
  compiled_expression dataf_;
};

class query_plan : public std::vector<minterm_plan> {
 public:
  std::string to_string() {
    std::string ret = "union(\n";
    for (auto& m : *this) {
      ret += "\t" + m.to_string() + ",\n";
    }
    ret += ")";
    return ret;
  }
};

}

#endif /* DIALOG_QUERY_PLAN_H_ */

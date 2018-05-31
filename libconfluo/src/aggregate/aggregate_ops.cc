#include "aggregate/aggregate_ops.h"

namespace confluo {

numeric sum_agg(const numeric &a, const numeric &b) {
  return a + b;
}

numeric min_agg(const numeric &a, const numeric &b) {
  return a < b ? a : b;
}

numeric max_agg(const numeric &a, const numeric &b) {
  return a < b ? b : a;
}

numeric count_agg(const numeric &a, const numeric &b) {
  return a + aggregators::count_one();
}

numeric invalid_agg(const numeric &a, const numeric &b) {
  throw invalid_operation_exception("Invalid aggregation performed.");
}

aggregate_type find_aggregator_id(const std::string &name) {
  std::string u_name = utils::string_utils::to_upper(name);
  for (unsigned int i = 0; i < aggregators::instance().size(); i++) {
    std::string a_name = utils::string_utils::to_upper(aggregators::instance()[i].name);
    if (u_name.compare(a_name) == 0) {
      return i;
    }
  }
  return 0;
}

}
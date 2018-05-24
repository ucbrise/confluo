#include "aggregate/aggregate_ops.h"

namespace confluo {

numeric count_one(UINT64_C(1));

aggregator invalid_aggregator = {"invalid", invalid_agg, invalid_agg, NONE_TYPE, numeric()};
aggregator sum_aggregator = {"sum", sum_agg, sum_agg, DOUBLE_TYPE, numeric(DOUBLE_TYPE, DOUBLE_TYPE.zero())};
aggregator min_aggregator = {"min", min_agg, min_agg, DOUBLE_TYPE, numeric(DOUBLE_TYPE, DOUBLE_TYPE.max())};
aggregator max_aggregator = {"max", max_agg, max_agg, DOUBLE_TYPE, numeric(DOUBLE_TYPE, DOUBLE_TYPE.min())};
aggregator count_aggregator = {"count", count_agg, sum_agg, ULONG_TYPE, numeric(ULONG_TYPE, ULONG_TYPE.zero())};
std::vector<aggregator>
    AGGREGATORS{invalid_aggregator, sum_aggregator, min_aggregator, max_aggregator, count_aggregator};

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
  return a + count_one;
}

numeric invalid_agg(const numeric &a, const numeric &b) {
  throw invalid_operation_exception("Invalid aggregation performed.");
}

aggregate_type find_aggregator_id(const std::string &name) {
  std::string u_name = utils::string_utils::to_upper(name);
  for (unsigned int i = 0; i < AGGREGATORS.size(); i++) {
    std::string a_name = utils::string_utils::to_upper(AGGREGATORS[i].name);
    if (u_name.compare(a_name) == 0) {
      return i;
    }
  }
  return 0;
}

}
#ifndef CONFLUO_TYPES_AGGREGATE_OPS_H_
#define CONFLUO_TYPES_AGGREGATE_OPS_H_

#include "types/numeric.h"

namespace confluo {

using aggregate_fn = numeric (*)(const numeric& v1, const numeric& v2);

struct aggregator {
  std::string name;
  aggregate_fn seq_op;
  aggregate_fn comb_op;
  data_type result_type;
  numeric zero;
};

// Standard aggregates: sum, min, max, count
static numeric count_one(UINT64_C(1));

/**
 * Sums two numerics
 * @param a First numeric
 * @param b Second numeric
 * @return The sum of the two numerics
 */
inline numeric sum_agg(const numeric& a, const numeric& b) {
  return a + b;
}

/**
 * Finds the min of two numerics
 * @param a First numeric
 * @param b Second numeric
 * @return The minimum of the two numerics
 */
inline numeric min_agg(const numeric& a, const numeric& b) {
  return a < b ? a : b;
}

/**
 * Finds the max of two numerics
 * @param a First numeric
 * @param b Second numeric
 * @return The maximum of the two numerics
 */
inline numeric max_agg(const numeric& a, const numeric& b) {
  return a < b ? b : a;
}

/**
 * Counts the numerics (equivalent to sum)
 * @param a First numeric 
 * @param b Second numeric
 * @return The sum of the two numerics
 */
inline numeric count_agg(const numeric& a, const numeric& b) {
  return a + count_one;
}

/**
 * Throws an exception for an invalid aggregate operation
 * @param a First numeric 
 * @param b Second numeric
 */
inline numeric invalid_agg(const numeric& a, const numeric& b) {
  throw invalid_operation_exception("Invalid aggregation performed.");
}

static aggregator invalid_aggregator = { "invalid", invalid_agg, invalid_agg,
    NONE_TYPE, numeric() };
static aggregator sum_aggregator = { "sum", sum_agg, sum_agg, DOUBLE_TYPE,
    numeric(DOUBLE_TYPE, DOUBLE_TYPE.zero()) };
static aggregator min_aggregator = { "min", min_agg, min_agg, DOUBLE_TYPE,
    numeric(DOUBLE_TYPE, DOUBLE_TYPE.max()) };
static aggregator max_aggregator = { "max", max_agg, max_agg, DOUBLE_TYPE,
    numeric(DOUBLE_TYPE, DOUBLE_TYPE.min()) };
static aggregator count_aggregator = { "count", count_agg, sum_agg, ULONG_TYPE,
    numeric(ULONG_TYPE, ULONG_TYPE.zero()) };

static std::vector<aggregator> AGGREGATORS { invalid_aggregator, sum_aggregator,
    min_aggregator, max_aggregator, count_aggregator };

typedef size_t aggregate_type;

aggregate_type find_aggregator_id(const std::string& name) {
  std::string uname = utils::string_utils::to_upper(name);
  for (unsigned int i = 0; i < AGGREGATORS.size(); i++) {
    std::string aname = utils::string_utils::to_upper(AGGREGATORS[i].name);
    if (uname.compare(aname) == 0) {
      return i;
    }
  }
  return 0;
}

}

#endif /* CONFLUO_TYPES_AGGREGATE_OPS_H_ */

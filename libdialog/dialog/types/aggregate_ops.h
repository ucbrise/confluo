#ifndef DIALOG_AGGREGATE_OPS_H_
#define DIALOG_AGGREGATE_OPS_H_

#include "aggregate_types.h"
#include "numeric.h"

namespace dialog {

using aggregate_fn = numeric (*)(const numeric& v1, const numeric& v2);
using zero_fn = numeric (*)(const data_type& type);

static numeric count_one(INT64_C(1));

struct aggregator {
  aggregate_fn agg;
  zero_fn zero;
};

// Standard aggregates: sum, min, max, count
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
  return a + b;
}

/**
 * Throws an exception for an invalid aggregate operation
 * @param a First numeric 
 * @param b Second numeric
*/ 
inline numeric invalid_agg(const numeric& a, const numeric& b) {
  throw invalid_operation_exception("Invalid aggregation performed.");
}

/**
 * Generates a numeric with the given type
 * @param type The type of the numeric
 * @return A numeric of the type type
 */
inline numeric sum_zero(const data_type& type) {
  return numeric(type, type.zero());
}

/**
 * Generates a numeric with a min zero
 */
inline numeric min_zero(const data_type& type) {
  return numeric(type, type.max());
}

inline numeric max_zero(const data_type& type) {
  return numeric(type, type.min());
}

/**
 * Generates a numeric with zero value of the type
 * @param type The specified data type
 * @return A numeric of the given type with value of zero in that type
 */
inline numeric count_zero(const data_type& type) {
  return numeric(type, type.zero());
}

inline numeric invalid_zero(const data_type& type) {
  throw invalid_operation_exception("Invalid zero op.");
}

static aggregator sum_aggregator = { sum_agg, sum_zero };
static aggregator min_aggregator = { min_agg, min_zero };
static aggregator max_aggregator = { max_agg, max_zero };
static aggregator count_aggregator = { count_agg, count_zero };
static aggregator invalid_aggregator = { invalid_agg, invalid_zero };

}

#endif /* DIALOG_AGGREGATE_OPS_H_ */

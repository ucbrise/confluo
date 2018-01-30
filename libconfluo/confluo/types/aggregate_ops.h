#ifndef CONFLUO_TYPES_AGGREGATE_OPS_H_
#define CONFLUO_TYPES_AGGREGATE_OPS_H_

#include "aggregate_types.h"
#include "numeric.h"

namespace confluo {

/** Function pointer for the aggregation function */
using aggregate_fn = numeric (*)(const numeric& v1, const numeric& v2);
/** The zero function pointer */
using zero_fn = numeric (*)(const data_type& type);

/** Step value for the data_type */
static numeric count_one(INT64_C(1));

/**
 * Encapsulation of an aggregate and zero function
 */
struct aggregator {
  /** Function pointer that does the aggregation */
  aggregate_fn agg;
  /** The function pointer representing zero for the data_type */
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
 * Generates a numeric with the maximum value the type has
 * @param type data_type that is used
 * @return numeric that contains the type and maximum value
 */
inline numeric min_zero(const data_type& type) {
  return numeric(type, type.max());
}

/**
 * Generates a numeric with the minimum value of a particular type
 * @param type the desired data_type
 * @return numeric containing the type and minimum value
 */
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

/**
 * Throws an exception for an invalid zero function
 * @param type data_type of the zero function
 * @throws invalid_operation_exception
 */
inline numeric invalid_zero(const data_type& type) {
  throw invalid_operation_exception("Invalid zero op.");
}

/** An aggregator that performs sum operation */
static aggregator sum_aggregator = { sum_agg, sum_zero };
/** An aggregator that performs min operation */
static aggregator min_aggregator = { min_agg, min_zero };
/** An aggregator that performs the max oepration */
static aggregator max_aggregator = { max_agg, max_zero };
/** An aggregator that performs the count operation */
static aggregator count_aggregator = { count_agg, count_zero };
/** An aggregator for invalid operations */
static aggregator invalid_aggregator = { invalid_agg, invalid_zero };

}

#endif /* CONFLUO_TYPES_AGGREGATE_OPS_H_ */

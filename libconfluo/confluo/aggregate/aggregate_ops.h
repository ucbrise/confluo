#ifndef CONFLUO_TYPES_AGGREGATE_OPS_H_
#define CONFLUO_TYPES_AGGREGATE_OPS_H_

#include "types/data_type.h"
#include "types/numeric.h"

namespace confluo {

/** Function pointer for the aggregation function */
using aggregate_fn = numeric (*)(const numeric &v1, const numeric &v2);

/**
 * Encapsulation of an aggregate and zero function
 */
struct aggregator {
  /** The name of the aggregator */
  std::string name;
  /** The sequential aggregate function */
  aggregate_fn seq_op;
  /** The combinational aggregate function */
  aggregate_fn comb_op;
  /** The result of the aggregator */
  data_type result_type;
  /** The numeric representing zero */
  numeric zero;
};

// Standard aggregates: sum, min, max, count
extern numeric count_one;

/**
 * Sums two numerics
 * @param a First numeric
 * @param b Second numeric
 * @return The sum of the two numerics
 */
numeric sum_agg(const numeric &a, const numeric &b);

/**
 * Finds the min of two numerics
 * @param a First numeric
 * @param b Second numeric
 * @return The minimum of the two numerics
 */
numeric min_agg(const numeric &a, const numeric &b);

/**
 * Finds the max of two numerics
 * @param a First numeric
 * @param b Second numeric
 * @return The maximum of the two numerics
 */
numeric max_agg(const numeric &a, const numeric &b);

/**
 * Counts the numerics (equivalent to sum)
 * @param a First numeric
 * @param b Second numeric
 * @return The sum of the two numerics
 */
numeric count_agg(const numeric &a, const numeric &b);

/**
 * Throws an exception for an invalid aggregate operation
 * @param a First numeric
 * @param b Second numeric
 */
numeric invalid_agg(const numeric &a, const numeric &b);

/**
 * The invalid aggregator
 */
extern aggregator invalid_aggregator;
/**
 * The sum aggregator
 */
extern aggregator sum_aggregator;
/**
 * The min aggregator
 */
extern aggregator min_aggregator;
/**
 * The max aggregator
 */
extern aggregator max_aggregator;
/**
 * The count aggregator.
 */
extern aggregator count_aggregator;

/**
 * A vector containing the aggregators
 */
extern std::vector<aggregator> AGGREGATORS;

/** The type of aggregate */
typedef size_t aggregate_type;

/**
 * Finds an aggregator from name
 *
 * @param name The name of the aggregator
 *
 * @return The matching aggregator
 */
aggregate_type find_aggregator_id(const std::string &name);

}

#endif /* CONFLUO_TYPES_AGGREGATE_OPS_H_ */

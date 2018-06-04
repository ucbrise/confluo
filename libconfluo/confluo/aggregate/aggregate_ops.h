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

class aggregators {
 public:
  static numeric &count_one() {
    static numeric count_one(UINT64_C(1));
    return count_one;
  }

  static aggregator &invalid_aggregator() {
    static aggregator agg{"invalid", invalid_agg, invalid_agg, data_type(0, 0), numeric()};
    return agg;
  }

  static aggregator &sum_aggregator() {
    static aggregator agg{"sum", sum_agg, sum_agg, primitive_types::DOUBLE_TYPE(),
                          numeric(primitive_types::DOUBLE_TYPE(), primitive_types::DOUBLE_TYPE().zero())};
    return agg;
  }

  static aggregator &min_aggregator() {
    static aggregator agg{"min", min_agg, min_agg, primitive_types::DOUBLE_TYPE(),
                          numeric(primitive_types::DOUBLE_TYPE(), primitive_types::DOUBLE_TYPE().max())};
    return agg;
  }

  static aggregator &max_aggregator() {
    static aggregator agg{"max", max_agg, max_agg, primitive_types::DOUBLE_TYPE(),
                          numeric(primitive_types::DOUBLE_TYPE(), primitive_types::DOUBLE_TYPE().min())};
    return agg;
  }

  static aggregator &count_aggregator() {
    static aggregator agg{"count", count_agg, sum_agg, primitive_types::ULONG_TYPE(),
                          numeric(primitive_types::ULONG_TYPE(), primitive_types::ULONG_TYPE().zero())};
    return agg;
  }

  static aggregators &instance() {
    static aggregators instance;
    return instance;
  }

  aggregator const &at(size_t i) const {
    return aggregators_[i];
  }

  aggregator &operator[](size_t i) {
    return aggregators_[i];
  }

  void push_back(const aggregator &agg) {
    aggregators_.push_back(agg);
  }

  void push_back(aggregator &&agg) {
    aggregators_.push_back(std::move(agg));
  }

  size_t size() const {
    return aggregators_.size();
  }

 private:
  aggregators() : aggregators_{aggregators::invalid_aggregator(), aggregators::sum_aggregator(),
                               aggregators::min_aggregator(), aggregators::max_aggregator(),
                               aggregators::count_aggregator()} {}

  std::vector<aggregator> aggregators_;
};

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

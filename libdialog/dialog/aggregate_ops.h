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
inline numeric sum_agg(const numeric& a, const numeric& b) {
  return a + b;
}

inline numeric min_agg(const numeric& a, const numeric& b) {
  return a < b ? a : b;
}

inline numeric max_agg(const numeric& a, const numeric& b) {
  return a < b ? b : a;
}

inline numeric count_agg(const numeric& a, const numeric& b) {
  return a + b;
}

inline numeric invalid_agg(const numeric& a, const numeric& b) {
  throw invalid_operation_exception("Invalid aggregation performed.");
}

inline numeric sum_zero(const data_type& type) {
  return numeric(type, type.zero());
}

inline numeric min_zero(const data_type& type) {
  return numeric(type, type.max());
}

inline numeric max_zero(const data_type& type) {
  return numeric(type, type.min());
}

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

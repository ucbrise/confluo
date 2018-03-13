#ifndef CONFLUO_AGGREGATE_AGGREGATE_MANAGER_H_
#define CONFLUO_AGGREGATE_AGGREGATE_MANAGER_H_

#include "aggregate_ops.h"

namespace confluo {

class aggregate_manager {
 public:
  static size_t register_aggregate(const aggregator& agg) {
    size_t id = AGGREGATORS.size();
    AGGREGATORS.push_back(agg);
    return id;
  }

  static aggregate_type get_aggregator_id(const std::string& name) {
    return find_aggregator_id(name);
  }

  static aggregator get_aggregator(const std::string& name) {
    return AGGREGATORS[get_aggregator_id(name)];
  }

  static aggregator get_aggregator(size_t id) {
    if (id < AGGREGATORS.size()) {
      return AGGREGATORS[id];
    }
    return AGGREGATORS[0];
  }

  static bool is_valid_id(size_t id) {
    return id >= 1 && id < AGGREGATORS.size();
  }
};

}

#endif /* CONFLUO_AGGREGATE_AGGREGATE_MANAGER_H_ */

#include "aggregate/aggregate_manager.h"

namespace confluo {

size_t aggregate_manager::register_aggregate(const aggregator &agg) {
  size_t id = AGGREGATORS.size();
  AGGREGATORS.push_back(agg);
  return id;
}

aggregate_type aggregate_manager::get_aggregator_id(const std::string &name) {
  return find_aggregator_id(name);
}

aggregator aggregate_manager::get_aggregator(const std::string &name) {
  return AGGREGATORS[get_aggregator_id(name)];
}

aggregator aggregate_manager::get_aggregator(size_t id) {
  if (id < AGGREGATORS.size()) {
    return AGGREGATORS[id];
  }
  return AGGREGATORS[0];
}

bool aggregate_manager::is_valid_id(size_t id) {
  return id >= 1 && id < AGGREGATORS.size();
}

}
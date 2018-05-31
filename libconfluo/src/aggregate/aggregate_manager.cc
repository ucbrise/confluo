#include "aggregate/aggregate_manager.h"

namespace confluo {

size_t aggregate_manager::register_aggregate(const aggregator &agg) {
  size_t id = aggregators::instance().size();
  aggregators::instance().push_back(agg);
  return id;
}

aggregate_type aggregate_manager::get_aggregator_id(const std::string &name) {
  return find_aggregator_id(name);
}

aggregator aggregate_manager::get_aggregator(const std::string &name) {
  return aggregators::instance()[get_aggregator_id(name)];
}

aggregator aggregate_manager::get_aggregator(size_t id) {
  if (id < aggregators::instance().size()) {
    return aggregators::instance()[id];
  }
  return aggregators::instance()[0];
}

bool aggregate_manager::is_valid_id(size_t id) {
  return id >= 1 && id < aggregators::instance().size();
}

}
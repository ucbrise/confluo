#include "trigger.h"

namespace confluo {

monitor::trigger::trigger(const std::string &name,
                          const std::string &aggregate_name,
                          reational_op_id op,
                          const numeric &threshold,
                          const uint64_t periodicity_ms)
    : name_(name),
      aggregate_name_(aggregate_name),
      op_(op),
      threshold_(threshold),
      periodicity_ms_(periodicity_ms),
      is_valid_(true) {
}

std::string monitor::trigger::name() const {
  return name_;
}

std::string monitor::trigger::expr() const {
  return aggregate_name_ + " " + relop_utils::op_to_str(op_) + " " + threshold_.to_string();
}

std::string monitor::trigger::aggregate_name() const {
  return aggregate_name_;
}

reational_op_id monitor::trigger::op() const {
  return op_;
}

const numeric monitor::trigger::threshold() const {
  return threshold_;
}

uint64_t monitor::trigger::periodicity_ms() const {
  return periodicity_ms_;
}

bool monitor::trigger::invalidate() {
  bool expected = true;
  return atomic::strong::cas(&is_valid_, &expected, false);
}

bool monitor::trigger::is_valid() const {
  return atomic::load(&is_valid_);
}

std::string monitor::trigger::to_string() const {
  return "{ name: \"" + name() + "\", expression: \"" + expr() + "\"}";
}

}
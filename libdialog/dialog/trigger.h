#ifndef DIALOG_TRIGGER_H_
#define DIALOG_TRIGGER_H_

#include "aggregate.h"

namespace dialog {
namespace monitor {
struct trigger {
 public:
  trigger(aggregate_id agg, size_t field_idx, const data_type& field_type,
          relop_id op, const numeric& threshold)
      : agg_id_(agg),
        field_idx_(field_idx),
        field_type_(field_type),
        op_(op),
        threshold_(threshold) {
  }

  aggregate create_aggregate() {
    return aggregate(field_type_, agg_id_);
  }

  aggregate_id agg_id() const {
    return agg_id_;
  }

  size_t field_idx() const {
    return field_idx_;
  }

  relop_id op() const {
    return op_;
  }

  const numeric threshold() const {
    return threshold_;
  }

 private:
  aggregate_id agg_id_;
  uint32_t field_idx_;
  data_type field_type_;
  relop_id op_;
  numeric threshold_;
};
}
}

#endif /* DIALOG_TRIGGER_H_ */

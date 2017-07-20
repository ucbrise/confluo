#ifndef DIALOG_TRIGGER_H_
#define DIALOG_TRIGGER_H_

namespace dialog {
namespace monitor {
struct trigger {
 public:
  trigger(uint32_t filter_id, relop_id op, const mutable_value& threshold)
      : filter_id_(filter_id),
        op_(op),
        threshold_(threshold) {
  }

  trigger(const trigger& other)
      : filter_id_(other.filter_id_),
        op_(other.op_),
        threshold_(other.threshold_) {
  }

  uint32_t filter_id() const {
    return filter_id_;
  }

  relop_id op() const {
    return op_;
  }

  const mutable_value threshold() const {
    return threshold_;
  }

 private:
  uint32_t filter_id_;
  relop_id op_;
  mutable_value threshold_;
};
}
}

#endif /* DIALOG_TRIGGER_H_ */

#ifndef DIALOG_AGGREGATE_H_
#define DIALOG_AGGREGATE_H_

#include "atomic.h"
#include "mutable_value.h"

namespace dialog {

enum aggregate_id: uint8_t {
  D_SUM = 0,
  D_MIN = 1,
  D_MAX = 2,
  D_COUNT = 3
};

using aggregate_fn = mutable_value_t (*)(const mutable_value_t& v1, const mutable_value_t& v2);
using zero_fn = mutable_value_t (*)(const data_type& type);

struct aggregator {
  aggregate_fn agg;
  zero_fn zero;
};

// Standard aggregates: sum, min, max, count
inline mutable_value_t sum_agg(const mutable_value_t& a, const mutable_value_t& b) {
  return a + b;
}

inline mutable_value_t min_agg(const mutable_value_t& a, const mutable_value_t& b) {
  return a < b ? a : b;
}

inline mutable_value_t max_agg(const mutable_value_t& a, const mutable_value_t& b) {
  return a < b ? b : a;
}

inline mutable_value_t count_agg(const mutable_value_t& a, const mutable_value_t& b) {
  return a + mutable_value_t(a.type(), a.type().one());
}

inline mutable_value_t sum_zero(const data_type& type) {
  return mutable_value_t(type, type.zero());
}

inline mutable_value_t min_zero(const data_type& type) {
  return mutable_value_t(type, type.max());
}

inline mutable_value_t max_zero(const data_type& type) {
  return mutable_value_t(type, type.min());
}

inline mutable_value_t count_zero(const data_type& type) {
  return mutable_value_t(type, type.zero());
}

static aggregator sum = { sum_agg, sum_zero };
static aggregator min = { min_agg, min_zero };
static aggregator max = { max_agg, max_zero };
static aggregator count = { count_agg, count_zero };

static std::array<aggregator, 4> aggregators { { sum, min, max, count } };

struct aggregate_node {
  aggregate_node(mutable_value_t agg, uint64_t version, aggregate_node *next)
      : value_(agg),
        version_(version),
        next_(next) {
  }

  inline mutable_value_t value() const {
    return value_;
  }

  inline uint64_t version() const {
    return version_;
  }

  inline aggregate_node* next() {
    return next_;
  }

 private:
  mutable_value_t value_;
  uint64_t version_;
  aggregate_node* next_;
};

class aggregate_t {
 public:
  /**
   * Constructor for aggregate
   *
   * @param agg Aggregate function pointer.
   */
  aggregate_t(data_type type, aggregator agg = sum)
      : head_(nullptr),
        agg_(agg),
        type_(type) {
  }

  /**
   * Constructor for aggregate
   *
   * @param id Aggregate ID.
   */
  aggregate_t(data_type type, aggregate_id id)
      : head_(nullptr),
        agg_(aggregators[id]),
        type_(type) {
  }

  /**
   * Default destructor.
   */
  ~aggregate_t() = default;

  /**
   * Get the aggregate value corresponding to the given version.
   *
   * @param version The version of the data store.
   * @return The aggregate value.
   */
  mutable_value_t get(uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    if (req != nullptr)
      return req->value();
    return agg_.zero(type_);
  }

  /**
   * Get update the aggregate value with given version.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void update(const mutable_value_t& value, uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    mutable_value_t old_agg = req == nullptr ? agg_.zero(type_) : req->value();
    aggregate_node *node = new aggregate_node(agg_.agg(old_agg, value), version,
                                              cur_head);
    atomic::store(&head_, node);
  }

 private:
  /**
   * Get node with largest version smaller than or equal to version.
   *
   * @param version The expected version for the node being searched for.
   * @return The node that satisfies the constraints above (if any), nullptr otherwise.
   */
  aggregate_node* get_node(aggregate_node *head, uint64_t version) {
    if (head == nullptr)
      return nullptr;

    aggregate_node *node = head;
    aggregate_node *ret = nullptr;
    uint64_t max_version = 0;
    while (node != nullptr) {
      if (node->version() == version)
        return node;

      if (node->version() < version && node->version() > max_version) {
        ret = node;
        max_version = node->version();
      }

      node = node->next();
    }
    return ret;
  }

  atomic::type<aggregate_node*> head_;
  aggregator agg_;
  data_type type_;
};

}

#endif /* DIALOG_AGGREGATE_H_ */

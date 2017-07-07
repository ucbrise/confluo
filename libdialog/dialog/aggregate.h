#ifndef DIALOG_AGGREGATE_H_
#define DIALOG_AGGREGATE_H_

#include "atomic.h"
#include "numeric.h"

namespace dialog {

enum aggregate_id {
  SUM = 0,
  MIN = 1,
  MAX = 2,
  COUNT = 3
};

using aggregate_fn = numeric_t (*)(const numeric_t& v1, const numeric_t& v2);
using zero_fn = numeric_t (*)(const data_type& type);

struct aggregator {
  aggregate_fn agg;
  zero_fn zero;
};

// Standard aggregates: sum, min, max, count
inline numeric_t sum_agg(const numeric_t& a, const numeric_t& b) {
  return a + b;
}

inline numeric_t min_agg(const numeric_t& a, const numeric_t& b) {
  return a < b ? a : b;
}

inline numeric_t max_agg(const numeric_t& a, const numeric_t& b) {
  return a < b ? b : a;
}

inline numeric_t count_agg(const numeric_t& a, const numeric_t& b) {
  return a + numeric_t(a.get_type(), a.get_type().one);
}

inline numeric_t sum_zero(const data_type& type) {
  return numeric_t(type, type.zero);
}

inline numeric_t min_zero(const data_type& type) {
  return numeric_t(type, type.max);
}

inline numeric_t max_zero(const data_type& type) {
  return numeric_t(type, type.min);
}

inline numeric_t count_zero(const data_type& type) {
  return numeric_t(type, type.zero);
}

static aggregator sum = { sum_agg, sum_zero };
static aggregator min = { min_agg, min_zero };
static aggregator max = { max_agg, max_zero };
static aggregator count = { count_agg, count_zero };

static std::array<aggregator, 4> aggregators { { sum, min, max, count } };

struct aggregate_node {
  aggregate_node(numeric_t agg, uint64_t version, aggregate_node *next)
      : value_(agg),
        version_(version),
        next_(next) {
  }

  inline numeric_t value() const {
    return value_;
  }

  inline uint64_t version() const {
    return version_;
  }

  inline aggregate_node* next() {
    return next_;
  }

 private:
  numeric_t value_;
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
  numeric_t get(uint64_t version) {
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
  void update(const numeric_t& value, uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    aggregate_node *req = get_node(cur_head, version);
    numeric_t old_agg = req == nullptr ? agg_.zero(type_) : req->value();
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

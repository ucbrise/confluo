#ifndef DIALOG_AGGREGATE_H_
#define DIALOG_AGGREGATE_H_

#include "atomic.h"
#include "numeric.h"

namespace dialog {

using aggregate_fn = numeric_t (*)(const numeric_t& v1, const numeric_t& v2);

// Standard aggregates: sum, min, max
numeric_t sum(const numeric_t& a, const numeric_t& b) {
  return a + b;
}

numeric_t min(const numeric_t& a, const numeric_t& b) {
  return a < b ? a : b;
}

numeric_t max(const numeric_t& a, const numeric_t& b) {
  return a < b ? b : a;
}

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
   * @param agg
   */
  aggregate_t(aggregate_fn agg)
      : agg_(agg) {
  }

  ~aggregate_t() = default;

  /**
   * Get the aggregate value corresponding to the given version.
   *
   * @param version The version of the data store.
   * @return The aggregate value.
   */
  numeric_t get(uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    return get_node(cur_head, version)->value();
  }

  /**
   * Get update the aggregate value with given version.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void update(const numeric_t& value, uint64_t version) {
    aggregate_node *cur_head = atomic::load(&head_);
    numeric_t old_agg = get_node(cur_head, version)->value();
    aggregate_node *node = new aggregate_node(agg_(old_agg, value), version,
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
    aggregate_node *node = head;
    aggregate_node *ret = nullptr;
    uint64_t max_version = 0;
    while (node) {
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
  aggregate_fn agg_;
};

}

#endif /* DIALOG_AGGREGATE_H_ */

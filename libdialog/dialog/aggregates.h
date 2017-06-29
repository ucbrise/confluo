#ifndef DIALOG_AGGREGATES_H_
#define DIALOG_AGGREGATES_H_

#include "atomic.h"

namespace dialog {
namespace aggregate {

// TODO: Add optimization to not read beyond read_tail.

template<typename T, typename update_functor>
struct aggregate_node {
  typedef aggregate_node<T, update_functor> aggregate_node_t;
  static_assert(std::is_integral<T>::value, "Aggregate must be integral");

  aggregate_node(T agg, T version, aggregate_node_t *next)
      : agg_(agg),
        version_(version),
        next_(next) {
  }

  void update_value(const T& value) {
    T expected = atomic::load(&agg_);
    while (!atomic::weak::cas(&agg_, expected,
                              update_functor::apply(expected, value)))
      ;
  }

  void unsafe_set(const T& value) {
    atomic::init(&agg_, value);
  }

  atomic::type<T> agg_;
  uint64_t version_;
  aggregate_node_t* next_;
};

template<typename T, typename update_functor>
class aggregate {
 public:
  typedef aggregate_node<T, update_functor> aggregate_node_t;

  aggregate() = default;
  ~aggregate() = default;

  /**
   * Get the aggregate value corresponding to the given version.
   *
   * @param version The version of the data store.
   * @return The aggregate value.
   */
  T get(uint64_t version) {
    aggregate_node_t *cur_head = atomic::load(&head_);
    return atomic::load(&get_node(cur_head, version)->agg_);
  }

  /**
   * Get update the aggregate value with given version.
   *
   * @param value The value with which the aggregate is to be updated.
   * @param version The aggregate version.
   */
  void update(const T& value, uint64_t version) {
    aggregate_node_t *cur_head = atomic::load(&head_);
    T old_agg = atomic::load(&get_node(cur_head, value)->agg_);
    aggregate_node_t *node = new aggregate_node_t(
        update_functor::apply(old_agg, value), version, cur_head);
    while (!atomic::weak::cas(&head_, node->next, node)) {
      old_agg = atomic::load(&get_node(node->next_, value)->agg_);
      node->unsafe_set(update_functor::apply(old_agg, value));
    }
  }

 private:

  /**
   * Get node with largest version smaller than or equal to version.
   *
   * @param version The expected version for the node being searched for.
   * @return The node that satisfies the constraints above (if any), nullptr otherwise.
   */
  aggregate_node_t* get_node(aggregate_node_t *head, uint64_t version) {
    aggregate_node_t *node = head;
    aggregate_node_t *ret = nullptr;
    uint64_t max_version = 0;
    while (node) {
      if (node->version_ == version)
        return node;

      if (node->version_ < version && node->version_ > max_version) {
        ret = node;
        max_version = node->version_;
      }

      node = node->next_;
    }
    return ret;
  }

  atomic::type<aggregate_node_t*> head_;
};

}
}

#endif /* DIALOG_AGGREGATES_H_ */
